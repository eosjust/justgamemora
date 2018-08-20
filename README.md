# mora
## tp接入：
### 首先获取tp的钱包列表，让用户选择使用哪一个钱包
```
tp.getWalletList('eos').then(console.log)
```

### 游戏数据结构：
  代币账户
```
  struct account
  {
    asset balance;//用户代币数量
  };
```

  所有游戏
```
  struct allgame
  {
    uint64_t id;//游戏id
    account_name player1;//玩家1，房主
    account_name player2;//玩家2，加入游戏的玩家
    account_name code;//发币的账户，justgamemora表示是自己，eosio.token表示是官方eos
    asset balance;//代币数量
    std::string guess1;//玩家1的猜拳
    std::string guess2;//玩家2的猜拳
    std::string coin1;//玩家1的投币
    std::string coin2;//玩家2的投币
    std::string randcoin;//系统产生的投币
    account_name winner;//判定的赢家
    time createtime;//游戏开始时间
    time starttime;//游戏开始时间
    time endtime;//游戏结束时间
  };
  ```
  
  ```
  opengame：待加入的游戏
  rungame：正在进行的游戏
  historygame：个人历史游戏
  mygame：个人当前游戏
  ```
以上五张game表结构一致

### 获取游戏状态：
以下获取状态，需要轮询刷新

1.获取自己的代币数量：
```
tp.getTableRows({
    json: true,
    code: 'justgamemora',
    scope: '$youraccount',
    table: 'accounts'
}).then(console.log)
```

2.获取所有游戏：
```
tp.getTableRows({
    json: true,
    code: 'justgamemora',
    scope: 'justgamemora',
    table: 'allgame'
}).then(console.log)
```
3.获取等待加入的游戏：
```
tp.getTableRows({
    json: true,
    code: 'justgamemora',
    scope: 'justgamemora',
    table: 'opengame'
}).then(console.log)
```
4.获取正在进行的游戏：
```
tp.getTableRows({
    json: true,
    code: 'justgamemora',
    scope: 'justgamemora',
    table: 'rungame'
}).then(console.log)
```
5.获取自己正在参与的游戏：
```
tp.getTableRows({
    json: true,
    code: 'justgamemora',
    scope: '$youraccount',
    table: 'mygame'
}).then(console.log)
```

6.获取自己历史上参与的游戏：
```
tp.getTableRows({
    json: true,
    code: 'justgamemora',
    scope: '$youraccount',
    table: 'historygame'
}).then(console.log)
```

### 游戏action接口

1.领取空投
```
tp.pushEosAction({
    actions: [
         {
            account: "justgamemora",
            name: "claimad",
            authorization: [
                {
                actor: '$youraccount',
                permission: "active"
                }
            ],
            data: {
                account: '$youraccount'
            }
        }
    ]
}).then(console.log);
```
2.新建游戏

免费新建
```
tp.pushEosAction({
    actions: [
         {
            account: "justgamemora",
            name: "playfree",
            authorization: [
                {
                actor: '$youraccount',
                permission: "active"
                }
            ],
            data: {
                player: '$youraccount',
                quantity: '100.0000 MORA',
                guess:'R,S,P',//三次出拳，逗号隔开，R石头，S剪刀，P布
                coin:'B'//B反面，F正面
            }
        }
    ]
}).then(console.log);
```
EOS新建
使用memo入参，格式为
method:guess;coin
```
tp.eosTokenTransfer({
    from: '$youraccount',
    to: 'justgamemora',
    amount: '0.0100',
    tokenName: 'EOS',
    precision: 4,
    contract: 'eosio.token',
    memo: 'playeos:R,R,R;B'
}).then(console.log);
```

3.加入游戏
免费加入
```
tp.pushEosAction({
    actions: [
         {
            account: "justgamemora",
            name: "joinfree",
            authorization: [
                {
                actor: '$youraccount',
                permission: "active"
                }
            ],
            data: {
                gameid: '$gameid',//从opengame表中获得的id
                player: '$youraccount',//自己既是player2
                guess:'R,S,P',//三次出拳，逗号隔开，R石头，S剪刀，P布
                coin:'B'//B反面，F正面
            }
        }
    ]
}).then(console.log);
```
eos加入
使用memo入参，格式为
method:gameid;guess;coin
示例：
```
tp.eosTokenTransfer({
    from: '$youraccount',
    to: 'justgamemora',
    amount: '0.0100',
    tokenName: 'EOS',
    precision: 4,
    contract: 'eosio.token',
    memo: 'joineos:1;R,R,R;B'
}).then(console.log);
```
4.审判游戏结果
```
tp.pushEosAction({
    actions: [
         {
            account: "justgamemora",
            name: "jurgegame",
            authorization: [
                {
                actor: '$youraccount',
                permission: "active"
                }
            ],
            data: {
                gameid: '$gameid',//从rungame表中获得的id
                user: '$youraccount'//为房主
            }
        }
    ]
}).then(console.log);
```


