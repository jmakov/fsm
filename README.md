# A simple implementation of a state machine

## Design decisions
- Based on benchmark this implementation is based on the CRTP visitor pattern.
- No internal/external events, no pre/post transition actions, no explicit guards other than preventing invalid 
transitions (see examples).
- The initial state is implicitly defined with the first element in `states`.

TODO:
- handle leveraged markets (e.g. margin calls)


## Examples
In `example/OrderFSM.hpp` we show how to implement this state graph of an order with IOT, IOK, GTD and GTC.

![State graph](assets/state_graph.jpg)