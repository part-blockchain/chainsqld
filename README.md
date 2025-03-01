English / [中文](doc/README_CN.md)

![Chainsql](/images/logo.png)

## Overview

ChainSQL is a blockchain supporting database operations. The DB operating log will be stored in the chainsql chain, then you can acquire the visual data in traditional DB.

## Details of ChainSQL
Detailed online-document can be found [here](http://docs.chainsql.net).

By setting the sync_tables, sync_db and auto_sync, you can restore the real DB tables you wanted from block chain. You can set the DB type arbitrarily, includeing mysql, sqlite, oracle and so on.

We defined three new transaction types for the DB operation, named sqlStatement, tableListSet and sqlTransaction. sqlStatement is used to insert, update or delete records, tableListSet is used to create a table and other operations to the table-self, sqlTransaction is used to operator a set of DB sql clause in one blockchain's transaction.

You can operate the DB or send DB operations to the block chain by the following four ways.

1. RPC API , supplied by the RPC modules.
2. Web sockets API, developed using javascript and java. Refer to [API in javascript](http://docs.chainsql.net/interface/nodeAPI.html) and [API in java](http://docs.chainsql.net/interface/javaAPI.html) .
3. Commandline, access to the node directly.
4. By kingshrad, using db's primitive sql clause. Refer to[Access by sql](http://www.chainsql.net/api_mysql.html).

The table module send data request to other nodes and sort the tx datas from other nodes, then give the right transaction data to the  sql module. This module also seeks every ledger in local block chain to get the compatible table data to send back to the required nodes.

The sql module analysis transaction data to get the real sql, then operate the DB ussing these real sql sentences.

Further more, storage modules make you check the DB before sending tx data to the block chain, this makes it possible that we can operate DB timely.

If you want to get more infomation about this production ,please access the site [www.chainsql.net](http://www.chainsql.net).

## Version
On updating  our version or  releasing new functions, the [RELEASENOTES](./RELEASENOTES.md) will be updated for the detail description.

## Setup
Refer to the  [Setup](./doc/manual/deploy.md) for details.

## Compile

Refer to the  [Builds](./Builds) directory for details, we introduce the detail compiling steps in several OS systems.

## License

ChainSQL is under the GNU General Public License v3.0. See the [LICENSE](./LICENSE) directory for details.

## Contact Us
Email: chainsql@peersafe.info

Wechat: scan the QR below to follow PeerSafe, and then send **chainsql**, you will receive the QR image for ChainSQL community.

PeerSafe Wechat QR code：

![PeerSafe](/images/peersafe.jpg)
