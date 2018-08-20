#include <algorithm>
#include <eosiolib/asset.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/db.h>
#include <eosiolib/eosio.hpp>
#include <string>
#include <vector>

#include <eosiolib/multi_index.hpp>
#include <eosiolib/print.hpp>
#include <time.h>

using namespace eosio;
using namespace std;

class mora : public eosio::contract {
private:
  //@abi table accounts i64
  struct account {
    asset balance;

    uint64_t primary_key() const { return balance.symbol.name(); }
  };
  typedef eosio::multi_index<N(accounts), account> accounts;
  //@abi table stat i64
  struct stat {
    asset supply;
    asset max_supply;
    account_name issuer;
    uint64_t primary_key() const { return supply.symbol.name(); }
    EOSLIB_SERIALIZE(stat, (supply)(max_supply)(issuer))
  };
  typedef eosio::multi_index<N(stat), stat> stats;

  //@abi table accstate i64
  struct accstate {
    account_name account;
    time last_airdrop_claim_time;
    asset eos_balance;

    uint64_t primary_key() const { return account; }

    EOSLIB_SERIALIZE(accstate, (account)(last_airdrop_claim_time)(eos_balance))
  };
  typedef eosio::multi_index<N(accstates), accstate> accstates;

  //@abi table allgame i64
  //@abi table opengame i64
  //@abi table rungame i64
  //@abi table historygame i64
  //@abi table mygame i64
  struct moragame {
    uint64_t id;
    account_name player1;
    account_name player2;
    account_name code;
    asset balance;
    std::string guess1;
    std::string guess2;
    std::string coin1;
    std::string coin2;
    std::string randcoin;
    account_name winner;

    time createtime;
    time starttime;
    time endtime;
    uint64_t primary_key() const { return id; }
    EOSLIB_SERIALIZE(
        moragame, (id)(player1)(player2)(code)(balance)(guess1)(guess2)(coin1)(
                      coin2)(randcoin)(winner)(createtime)(starttime)(endtime))
  };
  typedef eosio::multi_index<N(allgame), moragame> allgames;

  typedef eosio::multi_index<N(opengame), moragame> opengames;

  typedef eosio::multi_index<N(rungame), moragame> rungames;

  typedef eosio::multi_index<N(historygame), moragame> historygames;

  typedef eosio::multi_index<N(mygame), moragame> mygames;

  void sub_balance(account_name owner, asset value) {
    accounts from_acnts(_self, owner);
    const auto &from =
        from_acnts.get(value.symbol.name(), "no balance object found");
    eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");
    if (from.balance.amount == value.amount) {
      from_acnts.erase(from);
    } else {
      from_acnts.modify(from, owner, [&](auto &a) { a.balance -= value; });
    }
  }

  void add_balance(account_name owner, asset value, account_name ram_payer) {
    accounts to_acnts(_self, owner);
    auto to = to_acnts.find(value.symbol.name());
    if (to == to_acnts.end()) {
      to_acnts.emplace(ram_payer, [&](auto &a) { a.balance = value; });
    } else {
      to_acnts.modify(to, 0, [&](auto &a) { a.balance += value; });
    }
  }
  void mtransfer(account_name from, account_name to, asset quantity,
                 account_name ram_payer) {
    eosio_assert(is_account(to), "to account does not exist");
    sub_balance(from, quantity);
    add_balance(to, quantity, ram_payer);
  }

  //分割字符串
  void splitstring(const std::string &s, std::vector<std::string> &v,
                   const std::string &c) {
    std::string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while (std::string::npos != pos2) {
      v.push_back(s.substr(pos1, pos2 - pos1));

      pos1 = pos2 + c.size();
      pos2 = s.find(c, pos1);
    }
    if (pos1 != s.length())
      v.push_back(s.substr(pos1));
  }
  std::string joinString(std::vector<std::string> &v, const std::string &c) {
    if (v.size() > 0) {
      std::string ret = "";
      for (int i = 0; i < v.size(); i++) {
        ret += v[i];
        if (i != v.size() - 1) {
          ret += c;
        }
      }
      return ret;
    }
    return "";
  }

  //判决两个结果
  int jurge(vector<std::string> guess1, vector<std::string> guess2,
            std::string randcoin) {
    int ret = 0;
    for (int i = 0; i < 3; i++) {
      ret += jurgeOne(guess1[i], guess2[i]);
    }
    if (ret == 0) {
      if (guess1[3] == randcoin) {
        ret = 1;
      } else {
        ret = -1;
      }
    }
    return ret;
  }
  int jurgeOne(std::string A, std::string B) {
    // R石头，S剪子，P布。
    if ((A == "R" && B == "S") || (A == "S" && B == "P") ||
        (A == "P" && B == "R")) {
      return 1;
    } else if (A == B) {
      return 0;
    } else {
      return -1;
    }
  }
  //系统随机产生硬币正反面
  std::string getRandCoin() {
    time t = now();
    uint32_t it = (uint32_t)t;
    int p = it % 2;
    if (p == 0) {
      return "B";
    } else {
      return "F";
    }
  }
  //检查guess参数是否合法
  // TODO guess coin 检查
  int checkGuessLegal(std::string guess) {
    if (guess.size() !=5) {
      return 0;
    }
    std::vector<std::string> guessVector;
    splitstring(guess,guessVector,",");
    if(guessVector.size()!=3){
      return 0;
    }
    int ret = 1;
    for (int i = 0; i < guessVector.size(); i++) {
      std::string one = guessVector[i];
      if (one == "R" || one == "S" || one == "P") {
      } else {
        ret = 0;
      }
    }
    return ret;
  }

  int checkCoinLegal(std::string coin) {
    int ret = 1;
    if (coin.size() != 1) {
      return 0;
    }
    if (coin == "B" || coin == "F") {
    } else {
      ret = 0;
    }
    return ret;
  }

  uint64_t string2int(std::string str) {
    uint64_t x = stoull(str);
    return x;
  }
  std::string int2string(uint64_t val) { return std::to_string(val); }

  void opennewgame(account_name player, account_name code,
                   account_name rampayer, asset quantity, std::string guess,
                   std::string coin);

  void joingame(uint64_t gameid, account_name player, account_name code,
                account_name rampayer, asset quantity, std::string guess,
                std::string coin);
  void playeos(account_name player, account_name code, asset quantity,
               std::string argStr);

  void joineos(account_name player, account_name code, asset quantity,
               std::string argStr);

public:
  mora(account_name self) : contract(self) {}

  inline asset get_supply(symbol_name sym) const;
  inline asset get_balance(account_name owner, symbol_name sym) const;
  struct transfer_args {
    account_name from;
    account_name to;
    asset quantity;
    string memo;
  };

  void on(const currency::transfer &t, account_name code);
  void apply(account_name code, account_name action);
  void create(account_name issuer, asset maximum_supply);
  void issue(account_name to, asset quantity, string memo);
  void transfer(account_name from, account_name to, asset quantity,
                string memo);

  // @abi action
  void hi(account_name player1, account_name player2);

  // @abi action
  void eraseall(account_name user);
  // @abi action
  void claimad(account_name account);

  // @abi action
  void playfree(account_name player, asset quantity, std::string guess,
                std::string coin);
  // @abi action
  void joinfree(uint64_t gameid, account_name player, std::string guess,
                std::string coin);
  // @abi action
  void jurgegame(uint64_t gameid, account_name user);
};

asset mora::get_supply(symbol_name sym) const {
  stats statstable(_self, sym);
  const auto &st = statstable.get(sym);
  return st.supply;
}

asset mora::get_balance(account_name owner, symbol_name sym) const {
  accounts accountstable(_self, owner);
  const auto &ac = accountstable.get(sym);
  return ac.balance;
}