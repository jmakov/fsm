#ifndef BENCHMARKS_UTIL_RANDOM_HPP
#define BENCHMARKS_UTIL_RANDOM_HPP

#include <random>

namespace benchmarks::util {
    class RandomInInterval {
    private:
        std::random_device m_dev;
        std::mt19937 m_rng;
        std::uniform_int_distribution<std::mt19937::result_type> m_random_pick;
    public:
        RandomInInterval(const int start, const int stop): m_rng(m_dev()), m_random_pick(start, stop) {}

        int get_random_int() {
            return m_random_pick(m_rng);
        }
    };
}

#endif //BENCHMARKS_UTIL_RANDOM_HPP
