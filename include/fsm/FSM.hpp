#ifndef SRC_FSM_FINITESTATEMACHINE_HPP
#define SRC_FSM_FINITESTATEMACHINE_HPP
#include <optional>
#include <variant>
#include <utility>

namespace fsm {
    template<typename TChild, typename TVariants>
    class Fsm {
    public:
        template<typename Event>
        void process(Event&& event)
        {
            auto& child = static_cast<TChild&>(*this);
            // define transition map through different method signatures
            auto new_state = std::visit(
                    [&](auto& state) -> std::optional<TVariants> {
                        // state function goes in derived `transition` method
                        return child.transition(state, std::forward<Event>(event));
                        },
                    m_state);

            if(new_state) {
                m_state = *std::move(new_state);
            }
        }
    private:
        TVariants m_state;
    };
}
#endif //SRC_FSM_FINITESTATEMACHINE_HPP
