#pragma once

#include <steemit/app/applied_operation.hpp>
#include <steemit/app/state.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/steem_object_types.hpp>
#include <steemit/chain/history_object.hpp>

#include <steemit/tags/tags_plugin.hpp>

#include <steemit/follow/follow_plugin.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>
#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace steemit {
    namespace app {

        using namespace steemit::chain;
        using namespace steemit::protocol;

        struct order {
            price order_price;
            double real_price; // dollars per steem
            share_type steem;
            share_type sbd;
            fc::time_point_sec created;
        };

        struct order_book {
            std::vector<order> asks;
            std::vector<order> bids;
        };

        struct api_context;

        struct scheduled_hardfork {
            hardfork_version hf_version;
            fc::time_point_sec live_time;
        };

        struct liquidity_balance {
            std::string account;
            fc::uint128_t weight;
        };

        struct withdraw_route {
            std::string from_account;
            std::string to_account;
            uint16_t percent;
            bool auto_vest;
        };

        enum withdraw_route_type {
            incoming,
            outgoing,
            all
        };

        class database_api_impl;

/**
 * @class discussion_query
 * @brief The discussion_query structure implements the RPC API param set.
 *  Defines the arguments to a query as a struct so it can be easily extended
 */

        class discussion_query {
        public:
            void validate() const {
                FC_ASSERT(limit <= 100);

                for (const std::set<std::string>::value_type &iterator : filter_tags) {
                    FC_ASSERT(select_tags.find(iterator) ==
                              select_tags.end());
                }
            }

            uint32_t limit = 0; ///< the discussions return amount top limit
            std::set<std::string> select_authors; ///< list of authors to select
            std::set<std::string> select_tags; ///< list of tags to include, posts without these tags are filtered
            std::set<std::string> filter_tags; ///< list of tags to exclude, posts with these tags are filtered;
            uint32_t truncate_body = 0; ///< the amount of bytes of the post body to return, 0 for all
            optional<std::string> start_author; ///< the author of discussion to start searching from
            optional<std::string> start_permlink; ///< the permlink of discussion to start searching from
            optional<std::string> parent_author; ///< the author of parent discussion
            optional<std::string> parent_permlink; ///< the permlink of parent discussion
        };

/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
        class database_api {
        public:
            database_api(const steemit::app::api_context &ctx);

            ~database_api();

            ///////////////////
            // Subscriptions //
            ///////////////////

            void set_subscribe_callback(std::function<void(const variant &)> cb, bool clear_filter);

            void set_pending_transaction_callback(std::function<void(const variant &)> cb);

            void set_block_applied_callback(std::function<void(const variant &block_header)> cb);

            /**
             * @brief Stop receiving any notifications
             *
             * This unsubscribes from all subscribed markets and objects.
             */
            void cancel_all_subscriptions();

            std::vector<tag_api_obj> get_trending_tags(std::string after_tag, uint32_t limit) const;

            /**
             *  This API is a short-cut for returning all of the state required for a particular URL
             *  with a single query.
             */
            state get_state(std::string path) const;

            std::vector<category_api_obj> get_trending_categories(std::string after, uint32_t limit) const;

            std::vector<category_api_obj> get_best_categories(std::string after, uint32_t limit) const;

            std::vector<category_api_obj> get_active_categories(std::string after, uint32_t limit) const;

            std::vector<category_api_obj> get_recent_categories(std::string after, uint32_t limit) const;

            std::vector<account_name_type> get_active_witnesses() const;

            std::vector<account_name_type> get_miner_queue() const;

            /////////////////////////////
            // Blocks and transactions //
            /////////////////////////////

            /**
             * @brief Retrieve a block header
             * @param block_num Height of the block whose header should be returned
             * @return header of the referenced block, or null if no matching block was found
             */
            optional<block_header> get_block_header(uint32_t block_num) const;

            /**
             * @brief Retrieve a full, signed block
             * @param block_num Height of the block to be returned
             * @return the referenced block, or null if no matching block was found
             */
            optional<signed_block> get_block(uint32_t block_num) const;

            /**
             *  @brief Get sequence of operations included/generated within a particular block
             *  @param block_num Height of the block whose generated virtual operations should be returned
             *  @param only_virtual Whether to only include virtual operations in returned results (default: true)
             *  @return sequence of operations included/generated within the block
             */
            std::vector<applied_operation> get_ops_in_block(uint32_t block_num, bool only_virtual = true) const;

            /////////////
            // Globals //
            /////////////

            /**
             * @brief Retrieve compile-time constants
             */
            fc::variant_object get_config() const;

            /**
             * @brief Retrieve database unused memory amount
             */
            size_t get_free_memory() const;

            /**
             * @brief Return a JSON description of object representations
             * @return JSON description of object representations in a string
             */
            std::string get_schema() const;

            /**
             * @brief Retrieve the current @ref dynamic_global_property_object
             */
            dynamic_global_property_api_obj get_dynamic_global_properties() const;

            chain_properties get_chain_properties() const;

            price get_current_median_history_price() const;

            feed_history_api_obj get_feed_history() const;

            witness_schedule_api_obj get_witness_schedule() const;

            hardfork_version get_hardfork_version() const;

            scheduled_hardfork get_next_scheduled_hardfork() const;

            //////////
            // Keys //
            //////////

            std::vector<std::set<std::string>> get_key_references(std::vector<public_key_type> key) const;

            //////////////
            // Accounts //
            //////////////

            std::vector<extended_account> get_accounts(std::vector<std::string> names) const;

            /**
             *  @return all accounts that referr to the key or account id in their owner or active authorities.
             */
            std::vector<account_id_type> get_account_references(account_id_type account_id) const;

            /**
             * @brief Get a list of accounts by name
             * @param account_names Names of the accounts to retrieve
             * @return The accounts holding the provided names
             *
             * This function has semantics identical to @ref get_objects
             */
            std::vector<optional<account_api_obj>> lookup_account_names(const std::vector<std::string> &account_names) const;

            /**
             * @brief Get names and IDs for registered accounts
             * @param lower_bound_name Lower bound of the first name to return
             * @param limit Maximum number of results to return -- must not exceed 1000
             * @return Map of account names to corresponding IDs
             */
            std::set<std::string> lookup_accounts(const std::string &lower_bound_name, uint32_t limit) const;

            /**
             * @brief Get the total number of accounts registered with the blockchain
             */
            uint64_t get_account_count() const;

            std::vector<owner_authority_history_api_obj> get_owner_history(std::string account) const;

            optional<account_recovery_request_api_obj> get_recovery_request(std::string account) const;

            optional<escrow_api_obj> get_escrow(std::string from, uint32_t escrow_id) const;

            std::vector<withdraw_route> get_withdraw_routes(std::string account, withdraw_route_type type = outgoing) const;

            optional<account_bandwidth_api_obj> get_account_bandwidth(std::string account, bandwidth_type type) const;

            std::vector<savings_withdraw_api_obj> get_savings_withdraw_from(std::string account) const;

            std::vector<savings_withdraw_api_obj> get_savings_withdraw_to(std::string account) const;

            ///////////////
            // Witnesses //
            ///////////////

            /**
             * @brief Get a list of witnesses by ID
             * @param witness_ids IDs of the witnesses to retrieve
             * @return The witnesses corresponding to the provided IDs
             *
             * This function has semantics identical to @ref get_objects
             */
            std::vector<optional<witness_api_obj>> get_witnesses(const std::vector<witness_id_type> &witness_ids) const;

            std::vector<convert_request_api_obj> get_conversion_requests(const std::string &account_name) const;

            /**
             * @brief Get the witness owned by a given account
             * @param account The name of the account whose witness should be retrieved
             * @return The witness object, or null if the account does not have a witness
             */
            fc::optional<witness_api_obj> get_witness_by_account(std::string account_name) const;

            /**
             *  This method is used to fetch witnesses with pagination.
             *
             *  @return an array of `count` witnesses sorted by total votes after witness `from` with at most `limit' results.
             */
            std::vector<witness_api_obj> get_witnesses_by_vote(std::string from, uint32_t limit) const;

            /**
             * @brief Get names and IDs for registered witnesses
             * @param lower_bound_name Lower bound of the first name to return
             * @param limit Maximum number of results to return -- must not exceed 1000
             * @return Map of witness names to corresponding IDs
             */
            std::set<account_name_type> lookup_witness_accounts(const std::string &lower_bound_name, uint32_t limit) const;

            /**
             * @brief Get the total number of witnesses registered with the blockchain
             */
            uint64_t get_witness_count() const;

            ////////////
            // Market //
            ////////////

            /**
             * @breif Gets the current order book for STEEM:SBD market
             * @param limit Maximum number of orders for each side of the spread to return -- Must not exceed 1000
             */
            order_book get_order_book(uint32_t limit = 1000) const;

            std::vector<extended_limit_order> get_open_orders(std::string owner) const;

            /**
             * @breif Gets the current liquidity reward queue.
             * @param start_account The account to start the list from, or "" to get the head of the queue
             * @param limit Maxmimum number of accounts to return -- Must not exceed 1000
             */
            std::vector<liquidity_balance> get_liquidity_queue(std::string start_account, uint32_t limit = 1000) const;

            ////////////////////////////
            // Authority / validation //
            ////////////////////////////

            /// @brief Get a hexdump of the serialized binary form of a transaction
            std::string get_transaction_hex(const signed_transaction &trx) const;

            annotated_signed_transaction get_transaction(transaction_id_type trx_id) const;

            /**
             *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
             *  and return the minimal subset of public keys that should add signatures to the transaction.
             */
            std::set<public_key_type> get_required_signatures(const signed_transaction &trx, const flat_set<public_key_type> &available_keys) const;

            /**
             *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
             *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
             *  to get the minimum subset.
             */
            std::set<public_key_type> get_potential_signatures(const signed_transaction &trx) const;

            /**
             * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
             */
            bool verify_authority(const signed_transaction &trx) const;

            /*
             * @return true if the signers have enough authority to authorize an account
             */
            bool verify_account_authority(const std::string &name_or_id, const flat_set<public_key_type> &signers) const;

            /**
             *  if permlink is "" then it will return all votes for author
             */
            std::vector<vote_state> get_active_votes(std::string author, std::string permlink) const;

            std::vector<account_vote> get_account_votes(std::string voter) const;


            discussion get_content(std::string author, std::string permlink) const;

            std::vector<discussion> get_content_replies(std::string parent, std::string parent_permlink) const;

            /**
             * Used to retrieve top 1000 tags list used by an author sorted by most frequently used
             * @param author select tags of this author
             * @return vector of top 1000 tags used by an author sorted by most frequently used
             **/
            std::vector<pair<std::string, uint32_t>> get_tags_used_by_author(const std::string &author) const;

            /**
             * Used to retrieve the list of first payout discussions sorted by rshares^2 amount
             * @param query @ref discussion_query
             * @return vector of first payout mode discussions sorted by rshares^2 amount
             **/
            std::vector<discussion> get_discussions_by_trending(const discussion_query &query) const;

            /**
             * Used to retrieve the list of second payout discussions sorted by rshares^2 amount
             * @param query @ref discussion_query
             * @return vector of second payout mode discussions sorted by rshares^2 amount
             **/
            std::vector<discussion> get_discussions_by_trending30(const discussion_query &query) const;

            /**
             * Used to retrieve the list of discussions sorted by created time
             * @param query @ref discussion_query
             * @return vector of discussions sorted by created time
             **/
            std::vector<discussion> get_discussions_by_created(const discussion_query &query) const;

            /**
             * Used to retrieve the list of discussions sorted by last activity time
             * @param query @ref discussion_query
             * @return vector of discussions sorted by last activity time
             **/
            std::vector<discussion> get_discussions_by_active(const discussion_query &query) const;

            /**
             * Used to retrieve the list of discussions sorted by cashout time
             * @param query @ref discussion_query
             * @return vector of discussions sorted by last cashout time
             **/
            std::vector<discussion> get_discussions_by_cashout(const discussion_query &query) const;

            /**
             * Used to retrieve the list of discussions sorted by net rshares amount
             * @param query @ref discussion_query
             * @return vector of discussions sorted by net rshares amount
             **/
            std::vector<discussion> get_discussions_by_payout(const discussion_query &query) const;

            /**
             * Used to retrieve the list of discussions sorted by direct votes amount
             * @param query @ref discussion_query
             * @return vector of discussions sorted by direct votes amount
             **/
            std::vector<discussion> get_discussions_by_votes(const discussion_query &query) const;

            /**
             * Used to retrieve the list of discussions sorted by children posts amount
             * @param query @ref discussion_query
             * @return vector of discussions sorted by children posts amount
             **/
            std::vector<discussion> get_discussions_by_children(const discussion_query &query) const;

            /**
             * Used to retrieve the list of discussions sorted by hot amount
             * @param query @ref discussion_query
             * @return vector of discussions sorted by hot amount
             **/
            std::vector<discussion> get_discussions_by_hot(const discussion_query &query) const;

            /**
             * Used to retrieve the list of discussions from the feed of a specific author
             * @param query @ref discussion_query
             * @attention @ref discussion_query#select_authors must be set and must contain the @ref discussion_query#start_author param if the last one is not null
             * @return vector of discussions from the feed of authors in @ref discussion_query#select_authors
             **/
            std::vector<discussion> get_discussions_by_feed(const discussion_query &query) const;

            /**
             * Used to retrieve the list of discussions from the blog of a specific author
             * @param query @ref discussion_query
             * @attention @ref discussion_query#select_authors must be set and must contain the @ref discussion_query#start_author param if the last one is not null
             * @return vector of discussions from the blog of authors in @ref discussion_query#select_authors
             **/
            std::vector<discussion> get_discussions_by_blog(const discussion_query &query) const;

            std::vector<discussion> get_discussions_by_comments(const discussion_query &query) const;

            /**
             * Used to retrieve the list of discussions sorted by promoted balance amount
             * @param query @ref discussion_query
             * @return vector of discussions sorted by promoted balance amount
             **/
            std::vector<discussion> get_discussions_by_promoted(const discussion_query &query) const;


            /**
             *  Return the active discussions with the highest cumulative pending payouts without respect to category, total
             *  pending payout means the pending payout of all children as well.
             */
            std::vector<discussion> get_replies_by_last_update(account_name_type start_author, std::string start_permlink, uint32_t limit) const;


            /**
             *  This method is used to fetch all posts/comments by start_author that occur after before_date and start_permlink with up to limit being returned.
             *
             *  If start_permlink is empty then only before_date will be considered. If both are specified the eariler to the two metrics will be used. This
             *  should allow easy pagination.
             */
            std::vector<discussion> get_discussions_by_author_before_date(std::string author, std::string start_permlink, time_point_sec before_date, uint32_t limit) const;

            /**
             *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
             *  returns operations in the range [from-limit, from]
             *
             *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
             *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
             */
            std::map<uint32_t, applied_operation> get_account_history(std::string account, uint64_t from, uint32_t limit) const;

            ////////////////////////////
            // Handlers - not exposed //
            ////////////////////////////
            void on_api_startup();

        private:
            void set_pending_payout(discussion &d) const;

            void set_url(discussion &d) const;

            discussion get_discussion(comment_id_type, uint32_t truncate_body = 0) const;

            static bool filter_default(const comment_api_obj &c) {
                return false;
            }

            static bool exit_default(const comment_api_obj &c) {
                return false;
            }

            static bool tag_exit_default(const tags::tag_object &c) {
                return false;
            }

            template<typename Compare, typename Index, typename StartItr>
            std::multimap<tags::tag_object, discussion, Compare> get_discussions(const discussion_query &query,
                    const std::string &tag,
                    comment_id_type parent,
                    const Index &tidx, StartItr tidx_itr,
                    const std::function<bool(const comment_api_obj &)> &filter = &database_api::filter_default,
                    const std::function<bool(const comment_api_obj &)> &exit = &database_api::exit_default,
                    const std::function<bool(const tags::tag_object &)> &tag_exit = &database_api::tag_exit_default) const;

            comment_id_type get_parent(const discussion_query &q) const;

            void recursively_fetch_content(state &_state, discussion &root, std::set<std::string> &referenced_accounts) const;

            std::shared_ptr<database_api_impl> my;
        };
    }
}

FC_REFLECT(steemit::app::order, (order_price)(real_price)(steem)(sbd)(created));
FC_REFLECT(steemit::app::order_book, (asks)(bids));
FC_REFLECT(steemit::app::scheduled_hardfork, (hf_version)(live_time));
FC_REFLECT(steemit::app::liquidity_balance, (account)(weight));
FC_REFLECT(steemit::app::withdraw_route, (from_account)(to_account)(percent)(auto_vest));

FC_REFLECT(steemit::app::discussion_query, (select_tags)(filter_tags)(select_authors)(truncate_body)(start_author)(start_permlink)(parent_author)(parent_permlink)(limit));

FC_REFLECT_ENUM(steemit::app::withdraw_route_type, (incoming)(outgoing)(all));

FC_API(steemit::app::database_api,
// Subscriptions
        (set_subscribe_callback)
                (set_pending_transaction_callback)
                (set_block_applied_callback)
                (cancel_all_subscriptions)

                // tags
                (get_trending_tags)
                (get_tags_used_by_author)
                (get_discussions_by_trending)
                (get_discussions_by_trending30)
                (get_discussions_by_created)
                (get_discussions_by_active)
                (get_discussions_by_cashout)
                (get_discussions_by_payout)
                (get_discussions_by_votes)
                (get_discussions_by_children)
                (get_discussions_by_hot)
                (get_discussions_by_feed)
                (get_discussions_by_blog)
                (get_discussions_by_comments)
                (get_discussions_by_promoted)

                // Blocks and transactions
                (get_block_header)
                (get_block)
                (get_ops_in_block)
                (get_state)
                (get_trending_categories)
                (get_best_categories)
                (get_active_categories)
                (get_recent_categories)

                // Globals
                (get_config)
                (get_free_memory)
                (get_dynamic_global_properties)
                (get_chain_properties)
                (get_feed_history)
                (get_current_median_history_price)
                (get_witness_schedule)
                (get_hardfork_version)
                (get_next_scheduled_hardfork)

                // Keys
                (get_key_references)

                // Accounts
                (get_accounts)
                (get_account_references)
                (lookup_account_names)
                (lookup_accounts)
                (get_account_count)
                (get_conversion_requests)
                (get_account_history)
                (get_owner_history)
                (get_recovery_request)
                (get_escrow)
                (get_withdraw_routes)
                (get_account_bandwidth)
                (get_savings_withdraw_from)
                (get_savings_withdraw_to)

                // Market
                (get_order_book)
                (get_open_orders)
                (get_liquidity_queue)

                // Authority / validation
                (get_transaction_hex)
                (get_transaction)
                (get_required_signatures)
                (get_potential_signatures)
                (verify_authority)
                (verify_account_authority)

                // votes
                (get_active_votes)
                (get_account_votes)

                // content
                (get_content)
                (get_content_replies)
                (get_discussions_by_author_before_date)
                (get_replies_by_last_update)


                // Witnesses
                (get_witnesses)
                (get_witness_by_account)
                (get_witnesses_by_vote)
                (lookup_witness_accounts)
                (get_witness_count)
                (get_active_witnesses)
                (get_miner_queue)
)