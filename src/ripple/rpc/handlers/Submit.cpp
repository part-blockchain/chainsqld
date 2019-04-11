//------------------------------------------------------------------------------
/*
This file is part of rippled: https://github.com/ripple/rippled
Copyright (c) 2012-2014 Ripple Labs Inc.
Permission to use, copy, modify, and/or distribute this software for any
purpose  with  or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.
THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <BeastConfig.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/tx/apply.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/TransactionSign.h>
#include <ripple/app/main/Application.h>		
#include <ripple/app/ledger/LedgerMaster.h>		
#include <ripple/app/ledger/TransactionMaster.h>		
#include <ripple/app/misc/TxQ.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STParsedJSON.h>

namespace ripple {

	static NetworkOPs::FailHard getFailHard(RPC::Context const& context)
	{
		return NetworkOPs::doFailHard(
			context.params.isMember("fail_hard")
			&& context.params["fail_hard"].asBool());
	}

	// {
	//   tx_json: <object>,
	//   secret: <secret>
	// }
	Json::Value doSubmit(RPC::Context& context)
	{
		context.loadType = Resource::feeMediumBurdenRPC;

		if (!context.params.isMember(jss::tx_blob))
		{
			auto const failType = getFailHard(context);

			return RPC::transactionSubmit(
				context.params,
				failType,
				context.role,
				context.ledgerMaster.getValidatedLedgerAge(),
				context.app,
				RPC::getProcessTxnFn(context.netOps));
		}

		Json::Value jvResult;

		std::pair<Blob, bool> ret(strUnHex(context.params[jss::tx_blob].asString()));

		if (!ret.second || !ret.first.size())
			return rpcError(rpcINVALID_PARAMS);

		SerialIter sitTrans(makeSlice(ret.first));

		std::shared_ptr<STTx const> stpTrans;

		try
		{
			stpTrans = std::make_shared<STTx const>(std::ref(sitTrans));
		}
		catch (std::exception& e)
		{
			jvResult[jss::error] = "invalidTransaction";
			jvResult[jss::error_exception] = e.what();

			return jvResult;
		}


		{
			if (!context.app.checkSigs())
				forceValidity(context.app.getHashRouter(),
					stpTrans->getTransactionID(), Validity::SigGoodOnly);
			auto validity = checkValidity(context.app.getHashRouter(),
				*stpTrans, context.ledgerMaster.getCurrentLedger()->rules(),
				context.app.config());
			if (validity.first != Validity::Valid)
			{
				jvResult[jss::error] = "invalidTransaction";
				jvResult[jss::error_exception] = "fails local checks: " + validity.second;

				return jvResult;
			}
		}

		std::string reason;
		auto tpTrans = std::make_shared<Transaction>(
			stpTrans, reason, context.app);

		//for tx signed but no sequence 
		auto tx_json = tpTrans->getJson(0);
		if (!tx_json.isMember(jss::Sequence))
		{
			auto const srcAddressID = parseBase58<AccountID>(
				tx_json[jss::Account].asString());

			if (!srcAddressID)
			{
				jvResult[jss::error] = "invalidTransaction";
				jvResult[jss::error_exception] = "field Account parse error";

				return jvResult;
			}
			auto sequence = context.app.getMasterTransaction().getAccountSequence(*srcAddressID);
			if (sequence == 0)
			{
				return rpcError(rpcSRC_ACT_NOT_FOUND);
			}
			tx_json[jss::Sequence] = sequence;
		}

		//reconstruct Transcation
		STParsedJSONObject parsed(std::string(jss::tx_json), tx_json);
		if (parsed.object == boost::none)
		{
			jvResult[jss::error] = parsed.error[jss::error];
			jvResult[jss::error_exception] = parsed.error[jss::error_message];
			return jvResult;
		}

		std::shared_ptr<STTx> stpTransWithSequence;
		try
		{

			stpTransWithSequence = std::make_shared<STTx>(
				std::move(parsed.object.get()));
		}
		catch (std::exception& e)
		{
			jvResult[jss::error] = "invalidTransaction";
			jvResult[jss::error_exception] = e.what();

			return jvResult;
		}

		auto tpTransWithSequence = std::make_shared<Transaction>(
			stpTransWithSequence, reason, context.app);

		if (tpTransWithSequence->getStatus() != NEW)
		{
			jvResult[jss::error] = "invalidTransaction";
			jvResult[jss::error_exception] = "fails local checks: " + reason;

			return jvResult;
		}

		try
		{
			auto const failType = getFailHard(context);

			context.netOps.processTransaction(
				tpTransWithSequence, isUnlimited(context.role), true, failType);
		}
		catch (std::exception& e)
		{
			jvResult[jss::error] = "internalSubmit";
			jvResult[jss::error_exception] = e.what();

			return jvResult;
		}


		try
		{
			jvResult[jss::tx_json] = tpTransWithSequence->getJson(0);
			jvResult[jss::tx_blob] = strHex(
				tpTransWithSequence->getSTransaction()->getSerializer().peekData());

			TER result = tpTransWithSequence->getResult();
			if (temUNCERTAIN != result)
			{
				std::string sToken;
				std::string sHuman;

				transResultInfo(result, sToken, sHuman);

				jvResult[jss::engine_result] = sToken;
				jvResult[jss::engine_result_code] = result;
				jvResult[jss::engine_result_message] = sHuman;
			}

			return jvResult;
		}
		catch (std::exception& e)
		{
			jvResult[jss::error] = "internalJson";
			jvResult[jss::error_exception] = e.what();

			return jvResult;
		}
	}

} // ripple