#include "OrderFSM.hpp"


int main() {
    auto time_in_force = orderfsm::TimeInForce{
        true, false, false, true};
    auto account_manager = orderfsm::AccountManager(100, 100);
    auto order = orderfsm::OrderFSM<orderfsm::OrderType::LIMIT, orderfsm::OrderSide::BUY>(
            orderfsm::Exchange::Deribit,
            orderfsm::Market::BTCUSD,
            time_in_force,
            orderfsm::Strategy::RabateEater,
            1,
            account_manager,
            10,
            1
            );

    order.process(orderfsm::Event::PlaceOrderReqACK {});
    order.process(orderfsm::Event::OrderPlacedInOrderBook {});
    order.process(orderfsm::Event::PartiallyFilled {10});
    order.process(orderfsm::Event::PartiallyFilled {90});
    // from execution report we see that volume left for this order is 0 and generate event filled with volume 0
    // the same event can be used to fill the whole order
    order.process(orderfsm::Event::Filled{0});

    // this will throw an exception since the transition is not allowed
    order.process(orderfsm::Event::Filled{0});
}