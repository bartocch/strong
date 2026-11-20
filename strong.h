#ifndef STRONG_STRONG_H
#define STRONG_STRONG_H

#include <type_traits>
#include <utility>

namespace strong_details {
    /** Satisfied iff values of type `T` are supported by `strong`. */
    template <typename T>
    concept strong_value = (std::is_arithmetic_v<T>
        || (std::is_class_v<T> && !std::is_abstract_v<T>)   // non-abstract classes only
        || std::is_pointer_v<T>)
        && !std::is_reference_v<T>
        && !std::is_function_v<T>
        && !std::is_void_v<T>
        && !std::is_const_v<T>
        && !std::is_volatile_v<T>;
}

template <typename S>
using underlying_t = S::value_type;

class skill_access {
public:
    template <typename S>   // TODO: shouldn't compile when S isn't instance of strong
    static underlying_t<S>& get(S& s) {
        return s.value_;
    }

    template <typename S>   // TODO: shouldn't compile when S isn't instance of strong
    static const underlying_t<S>& get(const S& s) {
        return s.value_;
    }

    template <typename S>   // TODO: shouldn't compile when S isn't instance of strong
    static underlying_t<S>&& get(S&& s) {
        return s.value_;
    }
};

template <strong_details::strong_value T, typename Tag, typename... Skills>
class strong : public Skills::template mixin<strong<T, Tag, Skills...>>... {
public:
    friend skill_access;

    using value_type = T;
    using tag_type = Tag;

    strong()
        requires std::default_initializable<T>
        : value_() {}

    template <typename... Args>
    constexpr explicit strong(Args&&... args)
        : value_(std::forward<Args>(args)...) {}

    strong(strong<T, Tag, Skills...>& strong)
        requires std::copy_constructible<T>
        : value_(strong.value_) {}

    strong(strong<T, Tag, Skills...>&& strong)
        requires std::move_constructible<T>
        : value_(std::move(strong.value_)) {}

private:
    T value_;
};

#endif // STRONG_STRONG_H
