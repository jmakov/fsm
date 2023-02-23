#ifndef EXAMPLE_ORDERFSM_HPP
#define EXAMPLE_ORDERFSM_HPP
#include <iostream>

#include <fsm/FSM.hpp>


namespace orderfsm {
    enum Exchange : int {
        CME,
        Binance,
        Deribit
    };

    enum Market : int {
        BTCUSD
    };

    enum Strategy : int {
        IcebergPicker,
        FlashOrderEater,
        MomentIgnitionMaster,
        RabateEater,
        LiquidityEvaporator
    };

    enum OrderType : int {
        LIMIT
    };

    enum OrderSide : int {
        BUY,
        SELL
    };

    struct TimeInForce {
        const bool immediate_or_cancel{};
        const bool immediate_or_kill{};
        const bool good_till_cancel{};
        const bool good_till_date{};
    };

    struct State {
        struct Sent {}; // order has been sent
        struct Pending {};  // order received by exchange, in the queue to be processed by the matching engine
        struct Placed {};   // matching engine on the exchange processed request and placed order in the order book
        struct PendingCancel {};   // request for order cancellation received by exchange, not yet processed
        struct Cancelled {};    // order cancelled and pulled from the order book
        struct PendingModification {}; // modification request received by the exchange, not yet processed
        struct Filled {};   // whole order matched, producing a trade, order has no remaining volume in the order book
        struct FilledPartially {}; // part of the order matched, producing a trade, some order volume left in the order book
        struct Expired {};  // order removed from the order book because it's life expectancy came to an end
        struct Rejected {}; // order was rejected
    };

    // we generate events from parsed execution report
    struct Event {
        struct PlaceOrderReqACK {};
        struct PendingCancellationACK {};
        struct PendingModificationACK {};

        struct OrderPlacedInOrderBook {};
        struct ModifiedPlaced {const int price_new {}; const int volume_new {};};

        struct PartiallyFilled {const int volume {};};
        struct ModifiedPartiallyFilled: ModifiedPlaced {};

        // end states
        struct Filled: PartiallyFilled {};
        struct Rejected {};
        struct Cancelled {};
        struct Expired {};
    };

    using states = std::variant<
            State::Sent,    // initial state
            State::Pending,
            State::Placed,
            State::PendingCancel,
            State::Cancelled,
            State::PendingModification,
            State::Filled,
            State::FilledPartially,
            State::Expired,
            State::Rejected
    >;

    class AccountManager {
    public:
        int available_BTC{};
        int available_USD{};

        AccountManager(int available_BTC, int available_USD)
        : available_BTC(available_BTC), available_USD(available_USD) {}
    };

    template<OrderType TOrderType, OrderSide TOrderSide>
    class OrderFSMBase: public fsm::Fsm<OrderFSMBase<TOrderType, TOrderSide>, states> {
    public:
        const Exchange exchange_id{};
        const Market market_id{};
        const TimeInForce time_in_force{};
        const Strategy strategy_id{}; // strategy handling this order
        const int order_id{};
        AccountManager &account;
        int price {};
        int volume {};

        OrderFSMBase(const Exchange exchange_id,
                     const Market market_id,
                     const TimeInForce time_in_force,
                     const Strategy strategy_id,
                     const int order_id,
                     AccountManager &account,
                     int price,
                     int volume
        ) : exchange_id(exchange_id),
            market_id(market_id),
            time_in_force(time_in_force),
            strategy_id(strategy_id),
            order_id(order_id),
            account(account),
            price(price),
            volume(volume) {}

        template<typename TState, typename TEvent>
        std::optional<states> transition(TState&, const TEvent&) { throw std::logic_error("Transition not allowed"); }
        virtual State::Rejected transition(State::Sent&, const Event::Rejected&) = 0;
        virtual State::Pending transition(State::Sent&, const Event::PlaceOrderReqACK&) = 0;
        virtual State::PendingCancel transition(State::Pending&, const Event::PendingCancellationACK&) = 0;
        virtual State::PendingCancel transition(State::Placed&, const Event::PendingCancellationACK&) = 0;
        virtual State::PendingCancel transition(State::FilledPartially&, const Event::PendingCancellationACK&) = 0;
        virtual State::Placed transition(State::Pending&, const Event::OrderPlacedInOrderBook&) = 0;
        virtual State::Placed transition(State::PendingModification&, const Event::ModifiedPlaced& event_modified_placed) = 0;
        virtual State::FilledPartially transition(State::Placed&, const Event::PartiallyFilled& event_partially_filled) = 0;
        virtual State::FilledPartially transition(State::FilledPartially&, const Event::PartiallyFilled& event_partially_filled) = 0;
        virtual State::FilledPartially transition(State::PendingModification&, const Event::ModifiedPartiallyFilled& event_modify_partially_filled) = 0;
        virtual State::Filled transition(State::Placed&, const Event::Filled& event_filled) = 0;
        virtual State::Filled transition(State::FilledPartially&, const Event::Filled& event_filled) = 0;
        virtual State::Cancelled transition(State::PendingCancel&, const Event::Cancelled&) = 0;
        virtual State::Cancelled transition(State::Placed&, const Event::Cancelled&) = 0;
        virtual State::Cancelled transition(State::FilledPartially&, const Event::Cancelled&) = 0;
        virtual State::PendingModification transition(State::Placed&, const Event::PendingModificationACK &) = 0;
        virtual State::PendingModification transition(State::FilledPartially&, const Event::PendingModificationACK &) = 0;
        virtual State::Expired transition(State::PendingCancel&, const Event::Expired&) = 0;
        virtual State::Expired transition(State::Placed&, const Event::Expired&) = 0;
        virtual State::Expired transition(State::FilledPartially&, const Event::Expired&) = 0;
        virtual State::Expired transition(State::Pending&, const Event::Expired&) = 0;
        virtual State::Expired transition(State::PendingModification&, const Event::Expired&) = 0;
    };

    template<OrderType TOrderType, OrderSide TOrderSide>
    class OrderFSM : public OrderFSMBase<TOrderType, TOrderSide> {};

    template<>
    class OrderFSM<OrderType::LIMIT, OrderSide::BUY> : public OrderFSMBase<OrderType::LIMIT, OrderSide::BUY> {
    public:
        OrderFSM(const Exchange exchange_id,
                 const Market market_id,
                 const TimeInForce time_in_force,
                 const Strategy strategy_id,
                 const int order_id,
                 AccountManager &account,
                 int price,
                 int volume
        ) : OrderFSMBase<OrderType::LIMIT, OrderSide::BUY>(exchange_id,
                         market_id,
                         time_in_force,
                         strategy_id,
                         order_id,
                         account,
                         price,
                         volume) {
            account.available_USD -= volume * price;
        };

        // transitions to rejected state
        State::Rejected transition(State::Sent&, const Event::Rejected&) final {
            account.available_USD += volume * price;
            return State::Rejected{};
        }

        // transitions to pending state
        State::Pending transition(State::Sent&, const Event::PlaceOrderReqACK&) final {return State::Pending{};}

        // transitions to pending_cancellation state
        State::PendingCancel transition(State::Pending&, const Event::PendingCancellationACK&) final {
            return State::PendingCancel{};}
        State::PendingCancel transition(State::Placed&, const Event::PendingCancellationACK&) final {
            return State::PendingCancel{};}
        State::PendingCancel transition(State::FilledPartially&, const Event::PendingCancellationACK&) final {
            return State::PendingCancel{};}

        // transitions to placed state
        State::Placed transition(State::Pending&, const Event::OrderPlacedInOrderBook&) final {
            return State::Placed{};}
        State::Placed transition(State::PendingModification&, const Event::ModifiedPlaced& event_modified_placed) final {
            account.available_USD += (price * volume)
                    - (event_modified_placed.price_new * event_modified_placed.volume_new);
            price = event_modified_placed.price_new;
            volume = event_modified_placed.volume_new;
            return State::Placed{};}

        // transitions to partially filled state
        State::FilledPartially transition(State::Placed&, const Event::PartiallyFilled& event_partially_filled) final {
            volume -= event_partially_filled.volume;
            account.available_BTC += event_partially_filled.volume;
            return State::FilledPartially{};
        }
        // when volume_left = 0 in execution report, we'll trigger Event::Filled
        State::FilledPartially transition(State::FilledPartially&, const Event::PartiallyFilled& event_partially_filled) final {
            volume -= event_partially_filled.volume;
            account.available_BTC += event_partially_filled.volume;
            return State::FilledPartially{};
        }
        State::FilledPartially transition(State::PendingModification&, const Event::ModifiedPartiallyFilled& event_modify_partially_filled) final {
            account.available_USD += (price * volume)
                                     - (event_modify_partially_filled.price_new * event_modify_partially_filled.volume_new);
            price = event_modify_partially_filled.price_new;
            volume = event_modify_partially_filled.volume_new;
            return State::FilledPartially{};
        }

        // transitions to filled state
        // when volume_left = 0 in execution report, we'll trigger Event::Filled
        State::Filled transition(State::Placed&, const Event::Filled& event_filled) final {
            volume = 0;
            account.available_BTC += event_filled.volume;
            return State::Filled{};
        }
        // when volume_left = 0 in execution report, we'll trigger Event::Filled
        State::Filled transition(State::FilledPartially&, const Event::Filled& event_filled) final {
            volume = 0;
            account.available_BTC += event_filled.volume;
            return State::Filled{};
        }

        // transitions to cancelled state
        State::Cancelled transition(State::PendingCancel&, const Event::Cancelled&) final {
            account.available_USD += price * volume;
            return State::Cancelled{};
        }
        State::Cancelled transition(State::Placed&, const Event::Cancelled&) final {
            if (time_in_force.immediate_or_kill) {
                account.available_USD += price * volume;
            } else {
                throw std::logic_error("Transition not allowed");
            };
            return State::Cancelled{};
        }
        State::Cancelled transition(State::FilledPartially&, const Event::Cancelled&) final {
            if (time_in_force.immediate_or_cancel) {
                account.available_USD += price * volume;
            } else {
                throw std::logic_error("Transition not allowed");
            };
            return State::Cancelled{};
        }

        // transition to pending_modification state
        State::PendingModification transition(State::Placed&, const Event::PendingModificationACK &) final {
            return State::PendingModification{};
        }
        State::PendingModification transition(State::FilledPartially&, const Event::PendingModificationACK &) final {
            return State::PendingModification{};
        }

        // transition to expired state
        State::Expired transition(State::PendingCancel&, const Event::Expired&) final {
            if (time_in_force.good_till_date) {
                account.available_USD += price * volume;
            } else {
                throw std::logic_error("Transition not allowed");
            };
            return State::Expired{};
        }
        State::Expired transition(State::Placed&, const Event::Expired&) final {
            if (time_in_force.good_till_date) {
                account.available_USD += price * volume;
            } else {
                throw std::logic_error("Transition not allowed");
            };
            return State::Expired{};
        }
        State::Expired transition(State::FilledPartially&, const Event::Expired&) final {
            if (time_in_force.good_till_date) {
                account.available_USD += price * volume;
            } else {
                throw std::logic_error("Transition not allowed");
            };
            return State::Expired{};
        }
        State::Expired transition(State::Pending&, const Event::Expired&) final {
            if (time_in_force.good_till_date) {
                account.available_USD += price * volume;
            } else {
                throw std::logic_error("Transition not allowed");
            };
            return State::Expired{};
        }
        State::Expired transition(State::PendingModification&, const Event::Expired&) final {
            if (time_in_force.good_till_date) {
                account.available_USD += price * volume;
            } else {
                throw std::logic_error("Transition not allowed");
            };
            return State::Expired{};
        }
    };
}
#endif //EXAMPLE_ORDERFSM_HPP
