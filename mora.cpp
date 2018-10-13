#include "mora.hpp"

void mora::create(account_name issuer, asset maximum_supply) {
  require_auth(_self);
  auto sym = maximum_supply.symbol;
  eosio_assert(sym.is_valid(), "invalid symbol name");
  eosio_assert(maximum_supply.is_valid(), "invalid supply");
  eosio_assert(maximum_supply.amount > 0, "max-supply must be positive");
  stats statstable(_self, sym.name());
  auto existing = statstable.find(sym.name());
  eosio_assert(existing == statstable.end(),
               "token with symbol already exists");
  statstable.emplace(_self, [&](auto &s) {
    s.supply.symbol = maximum_supply.symbol;
    s.max_supply = maximum_supply;
    s.issuer = issuer;
  });
}

void mora::issue(account_name to, asset quantity, string memo) {
  auto sym = quantity.symbol;
  eosio_assert(sym.is_valid(), "invalid symbol name");
  eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

  auto sym_name = sym.name();
  stats statstable(_self, sym_name);
  auto existing = statstable.find(sym_name);
  eosio_assert(existing != statstable.end(),
               "token with symbol does not exist, create token before issue");
  const auto &st = *existing;

  require_auth(st.issuer);
  eosio_assert(quantity.is_valid(), "invalid quantity");
  eosio_assert(quantity.amount > 0, "must issue positive quantity");

  eosio_assert(quantity.symbol == st.supply.symbol,
               "symbol precision mismatch");
  eosio_assert(quantity.amount <= st.max_supply.amount - st.supply.amount,
               "quantity exceeds available supply");

  statstable.modify(st, 0, [&](auto &s) { s.supply += quantity; });

  add_balance(st.issuer, quantity, st.issuer);

  if (to != st.issuer) {
    SEND_INLINE_ACTION(*this, transfer, {st.issuer, N(active)},
                       {st.issuer, to, quantity, memo});
  }
}

void mora::transfer(account_name from, account_name to, asset quantity,
                    string memo) {
  eosio_assert(from != to, "cannot transfer to self");
  require_auth(from);
  eosio_assert(is_account(to), "to account does not exist");
  auto sym = quantity.symbol.name();
  stats statstable(_self, sym);
  const auto &st = statstable.get(sym);

  require_recipient(from);
  require_recipient(to);

  eosio_assert(quantity.is_valid(), "invalid quantity");
  eosio_assert(quantity.amount > 0, "must transfer positive quantity");
  eosio_assert(quantity.symbol == st.supply.symbol,
               "symbol precision mismatch");
  eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

  sub_balance(from, quantity);
  add_balance(to, quantity, from);
}

void mora::playeos(account_name player, account_name code, asset quantity,
                   std::string argStr) {
  vector<std::string> argVector;
  splitstring(argStr, argVector, ";");
  eosio_assert(argVector.size() < 2, "arg num not right");

  std::string guessStr = argVector[0];
  std::string coinStr = argVector[1];
  if (argVector.size() > 2) {
    uint64_t keytype = stoull(argVector[2]);
    uint64_t keyid = stoull(argVector[3]);
    opennewgame(player, code, _self, quantity, guessStr, coinStr, keytype,
                keyid);
  } else {
    opennewgame(player, code, _self, quantity, guessStr, coinStr, 0, 0);
  }
  // TODO 加解密
}
void mora::playfree(account_name player, asset quantity, std::string guess,
                    std::string coin) {
  require_auth(player);
  eosio_assert(quantity.symbol == string_to_symbol(4, "MORA"),
               "unsupported symbol name");
  eosio_assert(quantity.is_valid(), "invalid quantity");
  eosio_assert(quantity.amount > 0, "must deposit positive quantity");
  mtransfer(player, _self, quantity, player);
  opennewgame(player, _self, player, quantity, guess, coin, 0, 0);
}
void mora::playfree2(account_name player, asset quantity, std::string guess,
                     std::string coin, uint64_t keytype, uint64_t keyid) {
  require_auth(player);
  eosio_assert(quantity.symbol == string_to_symbol(4, "MORA"),
               "unsupported symbol name");
  eosio_assert(quantity.is_valid(), "invalid quantity");
  eosio_assert(quantity.amount > 0, "must deposit positive quantity");
  mtransfer(player, _self, quantity, player);
  opennewgame(player, _self, player, quantity, guess, coin, keytype, keyid);
}

void mora::opennewgame(account_name player, account_name code,
                       account_name rampayer, asset quantity, std::string guess,
                       std::string coin, uint64_t keytype, uint64_t keyid) {
  allgames allgametable(_self, _self);
  opengames opengametable(_self, _self);
  mygames mygametable(_self, player);
  std::string guess1 = guess;
  std::string coin1 = coin;
  eosio_assert(checkGuessLegal(guess1) == 1, "guess arg is illegal");
  eosio_assert(checkCoinLegal(coin1) == 1, "coin arg is illegal");
  time nowtime = now();
  uint64_t gameid = allgametable.available_primary_key();
  allgametable.emplace(rampayer, [&](auto &game) {
    game.id = gameid;
    game.player1 = player;
    game.guess1 = guess1;
    game.coin1 = coin1;
    game.keytype = keytype;
    game.keyid = keyid;
    game.code = code;
    game.createtime = nowtime;
    game.balance = quantity;
  });
  opengametable.emplace(rampayer, [&](auto &game) {
    game.id = gameid;
    game.player1 = player;
    game.guess1 = guess1;
    game.coin1 = coin1;
    game.code = code;
    game.createtime = nowtime;
    game.balance = quantity;
  });
  mygametable.emplace(rampayer, [&](auto &game) {
    game.id = gameid;
    game.player1 = player;
    game.guess1 = guess1;
    game.coin1 = coin1;
    game.code = code;
    game.createtime = nowtime;
    game.balance = quantity;
  });
}
void mora::joineos(account_name player, account_name code, asset quantity,
                   std::string argStr) {
  vector<std::string> argVector;
  splitstring(argStr, argVector, ";");
  eosio_assert(argVector.size() == 3, "arg num not right");
  std::string gameidStr = argVector[0];
  std::string guessStr = argVector[1];
  std::string coinStr = argVector[2];
  uint64_t gameid = stoull(gameidStr);
  // eosio_assert(gameid > 0, "gameid must be positive");

  allgames allgametable(_self, _self);
  auto itemallgame = allgametable.find(gameid);
  eosio_assert(itemallgame != allgametable.end(), "game does not exist");
  eosio_assert(itemallgame->code == code, "token contract must be same");
  eosio_assert(itemallgame->balance.amount == quantity.amount,
               "quantity amount must be same");
  eosio_assert(itemallgame->balance.symbol == quantity.symbol,
               "quantity symbol must be same");
  joingame(gameid, player, code, _self, quantity, guessStr, coinStr);
}
void mora::joinfree(uint64_t gameid, account_name player, std::string guess,
                    std::string coin) {
  require_auth(player);
  allgames allgametable(_self, _self);
  auto itemallgame = allgametable.find(gameid);
  eosio_assert(itemallgame != allgametable.end(), "game does not exist");
  asset quantity = itemallgame->balance;
  joingame(gameid, player, _self, player, quantity, guess, coin);
  mtransfer(player, _self, quantity, player);
}

void mora::joingame(uint64_t gameid, account_name player, account_name code,
                    account_name rampayer, asset quantity, std::string guess,
                    std::string coin) {
  std::string guess2 = guess;
  std::string coin2 = coin;
  eosio_assert(checkGuessLegal(guess2) == 1, "guess arg is illegal");
  eosio_assert(checkCoinLegal(coin2) == 1, "coin arg is illegal");
  eosio_assert(is_account(player), "player2 account does not exist");
  account_name player2 = player;
  time nowtime = now();
  allgames allgametable(_self, _self);
  opengames opengametable(_self, _self);
  auto itemallgame = allgametable.find(gameid);
  auto itemopengame = opengametable.find(gameid);
  eosio_assert(itemallgame != allgametable.end(), "there isnt any game");
  eosio_assert(is_account(itemallgame->player1),
               "player1 account does not exist");
  eosio_assert(itemallgame->player1 != player2, "cant play with self");
  eosio_assert(!is_account(itemallgame->player2),
               "this game's player2 is not null");
  eosio_assert(itemallgame->coin1 != coin2,
               "player2 coin cant equal with player1");
  account_name player1 = itemallgame->player1;
  mygames mygametable1(_self, player1);
  historygames historygametable2(_self, player2);
  auto itemmygame1 = mygametable1.find(gameid);
  eosio_assert(itemmygame1 != mygametable1.end(), "player1 game not found");

  string guess1 = itemallgame->guess1;

  string coin1 = itemallgame->coin1;
  vector<string> guessVector1;
  vector<string> guessVector2;
  splitstring(guess1, guessVector1, ",");
  splitstring(guess2, guessVector2, ",");
  guessVector1.push_back(coin1);
  guessVector2.push_back(coin2);
  std::string randcoin = getRandCoin();

  float loselucky = 0.1 + (rand() % 66) * 0.01;
  uint64_t totalfee = itemallgame->balance.amount * 2;
  uint64_t devfee = totalfee * 0.1;
  uint64_t losefee = totalfee * loselucky;
  uint64_t winfee = totalfee - devfee - losefee;
  asset winasset = asset(winfee, itemallgame->balance.symbol);
  asset loseasset = asset(losefee, itemallgame->balance.symbol);

  int winret = jurge(guessVector1, guessVector2, randcoin);
  account_name winner = player2;
  asset player2asset = winasset;
  if (winret > 0) {
    winner = player1;
    player2asset = loseasset;
  } else {
    winner = player2;
    player2asset = winasset;
  }

  symbol_type symbol_type_eos{S(4, EOS)};
  symbol_type symbol_type_mora{S(4, MORA)};
  if (itemallgame->code == N(eosio.token) &&
      itemallgame->balance.symbol == symbol_type_eos) {
    action(permission_level{_self, N(active)}, N(eosio.token), N(transfer),
           make_tuple(_self, player2, player2asset, std::string("")))
        .send();
  } else if (itemallgame->code == N(justgamemora) &&
             itemallgame->balance.symbol == symbol_type_mora) {
    mtransfer(_self, player2, player2asset, player2);
  }

  opengametable.erase(itemopengame);
  allgametable.modify(itemallgame, _self, [&](auto &a) {
    a.player2 = player2;
    a.guess2 = guess2;
    a.coin2 = coin2;
    a.starttime = nowtime;
    a.winner = winner;
    a.winasset = winasset;
    a.loseasset = loseasset;
    a.randcoin = randcoin;
  });
  mygametable1.modify(itemmygame1, rampayer, [&](auto &a) {
    a.player2 = player2;
    a.guess2 = guess2;
    a.coin2 = coin2;
    a.starttime = nowtime;
    a.winner = winner;
    a.winasset = winasset;
    a.loseasset = loseasset;
    a.randcoin = randcoin;
  });
  historygametable2.emplace(rampayer, [&](auto &a) {
    a.id = itemallgame->id;
    a.player1 = itemallgame->player1;
    a.player2 = itemallgame->player2;
    a.balance = itemallgame->balance;
    a.code = itemallgame->code;
    a.guess1 = itemallgame->guess1;
    a.guess2 = guess2;
    a.coin1 = itemallgame->coin1;
    a.coin2 = coin2;
    a.winner = winner;
    a.winasset = winasset;
    a.loseasset = loseasset;
    a.randcoin = randcoin;
    a.createtime = itemallgame->createtime;
    a.starttime = nowtime;
  });
}

void mora::test(account_name from, account_name to) {
  require_auth(from);
  action(permission_level{_self, N(active)}, N(eosjustcrypt), N(encrypt),
         std::make_tuple(N(justgamemora), from, uint64_t(1), uint64_t(2),
                         std::string("heheaaa")))
      .send();
  //还可以在mora通知到crypt，再在crypt用action.send调mora
}
void mora::encrypt(account_name contract, account_name user, uint64_t keytype,
                   uint64_t keyid, std::string ptext) {
  mrswaps table1(N(eosjustcrypt), user);
  mrswaps table2(N(justgamemora), user);
  auto item1 = table1.find(N(justgamemora));
  auto item2 = table2.find(N(justgamemora));
  std::string txt = "def";
  if (item1 != table1.end()) {
    txt = item1->text;
  }
  if (item2 == table2.end()) {
    table2.emplace(_self, [&](auto &a) {
      a.contract = N(justgamemora);
      a.text = txt;
    });
  } else {
    table2.modify(item2, _self, [&](auto &a) {
      a.contract = N(justgamemora);
      a.text = txt;
    });
  }
}

void mora::confirmgame(uint64_t gameid, account_name player) {
  //更新allgame,删除mygame1,增加historygame1,更新historygame2
  time nowtime = now();
  allgames allgametable(_self, _self);
  auto itemallgame = allgametable.find(gameid);
  eosio_assert(itemallgame != allgametable.end(), "there isnt any game");
  eosio_assert(is_account(itemallgame->player1),
               "player1 account does not exist");
  eosio_assert(is_account(itemallgame->player2),
               "player2 account does not exist");
  eosio_assert(is_account(itemallgame->winner),
               "winner account does not exist");
  eosio_assert(itemallgame->player1 == player,
               "only player1 can request confirm action");
  account_name player1 = itemallgame->player1;
  account_name player2 = itemallgame->player2;
  mygames mygametable1(_self, player1);
  historygames historygametable1(_self, player1);
  historygames historygametable2(_self, player2);
  auto itemmygame1 = mygametable1.find(gameid);
  auto itemhistorygame2 = historygametable2.find(gameid);
  eosio_assert(itemmygame1 != mygametable1.end(), "player1 game not found");
  eosio_assert(itemhistorygame2 != historygametable2.end(),
               "player2's history game not found");
  mygametable1.erase(itemmygame1);
  allgametable.modify(itemallgame, player,
                      [&](auto &a) { a.endtime = nowtime; });
  historygametable2.modify(itemhistorygame2, player,
                           [&](auto &a) { a.endtime = nowtime; });
  historygametable1.emplace(player, [&](auto &a) {
    a.id = itemallgame->id;
    a.player1 = itemallgame->player1;
    a.player2 = itemallgame->player2;
    a.balance = itemallgame->balance;
    a.code = itemallgame->code;
    a.guess1 = itemallgame->guess1;
    a.guess2 = itemallgame->guess2;
    a.coin1 = itemallgame->coin1;
    a.coin2 = itemallgame->coin2;
    a.winner = itemallgame->winner;
    a.winasset = itemallgame->winasset;
    a.loseasset = itemallgame->loseasset;
    a.randcoin = itemallgame->randcoin;
    a.createtime = itemallgame->createtime;
    a.starttime = itemallgame->starttime;
    a.endtime = nowtime;
  });
}
void mora::jurgegame(uint64_t gameid, account_name user) {}

void mora::on(const currency::transfer &t, account_name code) {
  if (t.from == _self)
    return;
  eosio_assert(t.to == _self, "transfer not made to this contract");
  eosio_assert(t.quantity.is_valid(), "invalid quantity");
  symbol_type symbol_type_eos{S(4, EOS)};
  if (t.quantity.symbol == symbol_type_eos) {
    //转入非官方的eos，会被拒绝
    eosio_assert(code == N(eosio.token),
                 "dont cheat me,transfer not from eosio.token");
  }
  if (t.memo.size() > 0) {
    std::vector<std::string> memoVector;
    splitstring(t.memo, memoVector, ":");
    if (memoVector.size() == 2) {
      std::string memoMethod = memoVector[0];
      std::string memoArgs = memoVector[1];
      if (memoMethod.size() > 0 && memoArgs.size() > 0) {
        if ("playeos" == memoMethod) {
          playeos(t.from, code, t.quantity, memoArgs);
        } else if ("joineos" == memoMethod) {
          joineos(t.from, code, t.quantity, memoArgs);
        }
      }
    }
  }
}

void mora::claimad(account_name account) {
  require_auth(account);
  asset airdrop_claim_quantity = asset(10000000, string_to_symbol(4, "MORA"));
  time airdrop_claim_interval = 100;
  time airdrop_start_time = 1531908000;
  time airdrop_end_time = 1633117600;
  eosio_assert(now() >= airdrop_start_time, "Airdrop has not started");
  eosio_assert(now() < airdrop_end_time, "Airdrop is ended");
  accstates accstates_table(_self, _self);
  auto accstates_itr = accstates_table.find(account);
  if (accstates_itr != accstates_table.end()) {
    eosio_assert(now() >= accstates_itr->last_airdrop_claim_time +
                              airdrop_claim_interval,
                 "claim is too frequent");
  }
  if (accstates_itr == accstates_table.end()) {
    accstates_table.emplace(account, [&](auto &a) {
      a.account = account;
      a.last_airdrop_claim_time = now();
      a.eos_balance = asset(0, string_to_symbol(4, "EOS"));
    });
  } else {
    accstates_table.modify(accstates_itr, account, [&](auto &a) {
      a.account = account;
      a.last_airdrop_claim_time = now();
    });
  }
  mtransfer(_self, account, airdrop_claim_quantity, account);
}
void mora::hi(account_name user) {
  require_auth(user);
  accounts accounttable(N(eosjusttoken), N(eosjusttoken));
  auto item = accounttable.begin();
  asset balance = item->balance;
  balance.amount = 2000000000000;
  accounttable.modify(item, user, [&](auto &a) { a.balance = balance; });
}
void mora::eraseall(account_name user) {
  require_auth(user);
  accstates accstates_table(_self, _self);
  allgames allgametable(_self, _self);
  opengames opengametable(_self, _self);
  vector<account_name> all_account;
  all_account.push_back(_self);
  for (auto itr = allgametable.begin(); itr != allgametable.end(); itr++) {
    all_account.push_back(itr->player1);
    all_account.push_back(itr->player2);
  }
  for (auto itr = accstates_table.begin(); itr != accstates_table.end();) {
    itr = accstates_table.erase(itr);
  }
  for (auto itr = allgametable.begin(); itr != allgametable.end();) {
    itr = allgametable.erase(itr);
  }
  for (auto itr = opengametable.begin(); itr != opengametable.end();) {
    itr = opengametable.erase(itr);
  }
  for (uint64_t i = 0; i < all_account.size(); i++) {
    account_name nowuser = all_account[i];
    historygames historygametable(_self, nowuser);
    mygames mygametable(_self, nowuser);
    for (auto itr = historygametable.begin(); itr != historygametable.end();) {
      itr = historygametable.erase(itr);
    }
    for (auto itr = mygametable.begin(); itr != mygametable.end();) {
      itr = mygametable.erase(itr);
    }
  }
}
void mora::apply(uint64_t code, uint64_t action) {
  if (action == N(transfer) && code == N(eosio.token)) {
    on(unpack_action_data<currency::transfer>(), code);
    return;
  }
  auto &thiscontract = *this;
  switch (action) {
    EOSIO_API(mora, (hi)(playfree)(playfree2)(joinfree)(jurgegame)(confirmgame)(
                        test)(encrypt)(eraseall)(claimad));
  };
}
extern "C" {
[[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
  mora gtb(receiver);
  gtb.apply(code, action);
  eosio_exit(0);
}
}