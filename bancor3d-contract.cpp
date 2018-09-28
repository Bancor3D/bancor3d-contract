#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <math.h>

using eosio::action;
using eosio::asset;
using eosio::const_mem_fun;
using eosio::indexed_by;
using eosio::name;
using eosio::permission_level;
using eosio::print;
using eosio::string_to_name;
using std::string;
typedef long double real_type;

/**
 *
 *  Bancor3D.com - An Accelated Economic Game
 *
 *                      **   **
 *                 **************  *****
 *              **$$$$  ****   $$*******
 *            * $$$$$$$$$   $$* **   *$**
 *           *  $    $$$$$* $* *$$$$$$$$$**
 *          *  $$$$$$$$$$$$$ $$$$$$$$$**$$*
 *          $$  $$$$$$$ $$$$$$$$ $$$$$$$$$$$$
 *        *$ *$$$$$$$$$$$*$*$*$    *****$***
 *      **$$ $$$$$$*$ **    [[[[[[[   [[[
 *      **$**$$**$$$@@     [  #####  ## #
 *    *$$$*****$$$$$$         ..###   ###
 *    **$$$$$$$$ [[$$ ##        +++   ###
 *     $$$$$$**[[  [[$$    #          ##
 *       $$$***#[  #     ###      #   ##
 *       $$$$$$ ##++ #+        ## ######
 *       $$$$$#  ###       ##       ##
 *       $$$$$#   ##            ######
 *         $$$### ##      ##    #####
 *           $#$ #  +##  #   ##  ## #
 *           $$$$     #   +++#######
 *           $$*      ##############
 *           *$$# #  ++     ++++        
 *
 *  Bancor3D.com - An Accelated Economic Game
 *  Core Code as follows
 **/
class bancor3d : public eosio::contract {
  const std::string TEAM_ACCOUNT;
  const uint64_t KEY_UNIT = 10000;
  const uint64_t EOS_UNIT = 10000;
  const uint64_t TIME_INTERVAL = 3600 * 24 * 7; 

  public:
    bancor3d(account_name self):eosio::contract(self), users(_self, _self), games(_self, _self){}
    
    uint64_t eos_days_hodl(uint64_t k, uint64_t t)
    {
        uint64_t keys = k / KEY_UNIT;
        uint64_t time_in_h = t / 3600 + 1;
        return keys * time_in_h;
    }

    //@abi action
    void bigbang()
    {
        auto gameitr = games.begin();
        eosio_assert( gameitr != games.end(), "Are you sure? The game does not exist." );

        uint64_t t_now = now();
        eosio_assert( gameitr->t < t_now, "Game has not finished yet." );
        eosio_assert( gameitr->p > 0, "Insufficient jackpot." );

        uint64_t user_payout = 0;
        double game_weight = 0;
        uint64_t user_payout;
        eosio_assert( gameitr->b >= user_payout, "Insufficient money for users." );
        user_get_bonus(user_payout);
    }

    //@abi action
    void jackpot()
    {
        auto gameitr = games.begin();
        eosio_assert( gameitr != games.end(), "Are you sure? The game does not exist." );
        eosio_assert( gameitr->d >= 1000, "Insufficient jackpot dividends - minimum 0.1 EOS." );

        double game_weight = get_bonus(); // calcualte based on HODL Time

        eosio_assert( game_weight > 0, "No one deserves the jackpot dividends." );
        user_get_bonus(game_weight);

        games.modify( gameitr,0, [&]( auto& s ) {
            s.d = 0;
        });
    }

    //@abi action
    void withdraw( const account_name to) {
        require_auth( to );

        auto useritr = users.find( to );
        eosio_assert(useritr != users.end(), "Unknown account from Keynes' dictionary.");
        eosio_assert(useritr->p >= 1000, "Keynes does not like small change less than 0.1 EOS." );
        auto gameitr = games.begin();
        eosio_assert( gameitr != games.end(), "Are you sure? The game does not exist." );

        asset balance(useritr->p,S(4,EOS));
        users.modify( useritr, 0, [&]( auto& s ) {
            s.p = 0;
        });

        action(
            permission_level{ _self, N(active) },
            N(eosio.token), N(transfer),
            std::make_tuple(_self, to, balance, std::string("Good bye! - says Keynes. https://Bancor3D.com "))
        ).send();
    }

    void transfer( account_name from, account_name to, asset quantity, std::string memo ) {
        require_auth( from );
        
        eosio_assert( now() >= INIT_TIME, "Game not started yet."); 
        if(quantity.is_valid() && quantity.symbol == S(4, EOS) && from != _self && to == _self)
        {
            if(quantity.amount >= 10){
                eosio_assert( quantity.amount >= 1000, "Minimum ticket to Keynes experiment is 0.1 EOS." );
                eosio_assert( quantity.amount <= 10000*10000, "More than 10,000 EOS will make the designer run away." );
                eosio_assert( memo.size() <= 100 && memo.size() >=1, "Say something. Memo length should be less than 100 and greater than 1." );

                string amount;
                string refer_account;

                games.modify( gameitr, 0, [&]( auto& s ) {
                    s.e += jackpot;
                    s.k += key;
                    s.f += fee;
                    s.r += referrer;
                    s.p += pot;
                    s.b += eos;
                    s.t += get_time();
                });

                auto teamAccount = string_to_name(TEAM_ACCOUNT.c_str());
                auto teamitr = users.find( teamAccount );
                if( teamitr == users.end())
                {
                    teamitr = users.emplace( _self, [&]( auto& s ) {
                        s.n = teamAccount;
                        s.r = 0;
                        s.e = 0;
                        s.d = 0;
                        s.k = 0;
                        s.p = 0;
                        s.t = 0;
                        s.lt = 0;
                        s.lk = 0;
                    });
                }
                users.modify( teamitr,0, [&]( auto& s ) {
                    s.p += fee;
                });

                update_refer_bonus();
            }
        }
    }
    
    //@abi action
    void sell(account_name from, asset quantity)
    {
        require_auth( from );

        eosio_assert( quantity.is_valid(), "Invalid KEY quantity.");
        eosio_assert( quantity.amount >= KEY_UNIT, "Invalid KEY amount - at least 1 KEY." );
        eosio_assert( quantity.amount <= 1000000 * KEY_UNIT, "Invalid KEY amount - at most 1M KEY." );

        auto useritr = users.find( from );
        eosio_assert( useritr != users.end(), "Unknown account from Keynes' dictionary.");
        eosio_assert( useritr->k >= quantity.amount, "You don't have enough KEY .");

        double div = (double(useritr->e) - double(useritr->d)) * ratio;
        eosio_assert( ratio > 0 && ratio <= 1 , "Selling ratio should be (0, 1].");
        eosio_assert( useritr->e >= useritr->d && div >= 0 && useritr->e >= div , "Divident should be positive.");

        users.modify( useritr, 0, [&]( auto& s ) {
            s.k -= quantity.amount;
            s.p += eos;
            s.d += uint64_t(floor(div));
            if (s.k == 0) {
                s.t = 0;
            }
        });

        games.modify( gameitr, 0, [&]( auto& s ) {
            s.k -= quantity.amount;
            s.d += v;
            s.p -= v;
        });
    }

  private:
    // @abi table games i64
    struct game{
      uint64_t i;
      uint64_t k;
      uint64_t e;
      uint64_t f;
      uint64_t r;
      uint64_t p;
      uint64_t d;
      uint64_t b;
      uint64_t t;
      uint64_t primary_key() const { return i; }
      EOSLIB_SERIALIZE(game, (i)(k)(e)(f)(r)(p)(d)(b)(t))
    };
    typedef eosio::multi_index<N(games), game> game_list;
    game_list games;
    
    // @abi table users i64
    struct user {
      account_name n;
      account_name r;
      uint64_t e;
      uint64_t d;
      uint64_t k;
      uint64_t p;
      uint64_t t;
      uint64_t lt;
      uint64_t lk;
      uint64_t primary_key() const { return n; }
      uint64_t get_key() const { return k; }
      EOSLIB_SERIALIZE(user, (n)(r)(e)(d)(k)(p)(t)(lt)(lk))
    };
    typedef eosio::multi_index<N(users), user,
    indexed_by<N(k), const_mem_fun<user, uint64_t, &user::get_key>>
    > user_list;
    user_list users;
};

 #define EOSIO_ABI_EX( TYPE, MEMBERS ) \
 extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
       if( action == N(onerror)) { \
          eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); \
       } \
       auto self = receiver; \
       if((code == N(eosio.token) && action == N(transfer)) || (code == self && (action==N(sell) || action == N(bigbang) || action == N(jackpot) || action == N(withdraw) || action == N(onerror))) ) { \
          TYPE thiscontract( self ); \
          switch( action ) { \
             EOSIO_API( TYPE, MEMBERS ) \
          } \
       } \
    } \
 }

EOSIO_ABI_EX(bancor3d, (transfer)(sell)(bigbang)(jackpot)(withdraw))

