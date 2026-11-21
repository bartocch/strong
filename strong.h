#ifndef STRONG_STRONG_H
#define STRONG_STRONG_H

#include <algorithm>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

namespace strong_details {
    /** Satisfied iff values of type `T` are supported by `strong`. */
    template <typename T>
    concept strong_value =
        (std::is_arithmetic_v<T> ||
         (std::is_class_v<T> &&
          !std::is_abstract_v<T>) // non-abstract classes only
         || std::is_pointer_v<T>) &&
        !std::is_const_v<T> && !std::is_volatile_v<T>;
} // namespace strong_details

template <typename S>
using underlying_t = S::value_type;

class skill_access {
public:
    // TODO: shouldn't compile when S isn't instance of strong
    template <typename S>
    static underlying_t<S>& get(S& s) {
        return s.value_;
    }
    // TODO: shouldn't compile when S isn't instance of strong
    template <typename S>
    static const underlying_t<S>& get(const S& s) {
        return s.value_;
    }

    // TODO: shouldn't compile when S isn't instance of strong
    template <typename S>
    static underlying_t<S>&& get(S&& s) {
        return s.value_;
    }
};

template <strong_details::strong_value T, typename Tag, typename... Skills>
class strong : public Skills::template mixin<strong<T, Tag, Skills...>>... {
    T value_;

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

    strong(strong& strong)
        requires std::copy_constructible<T>
        : value_(strong.value_) {}

    strong(strong&& strong) noexcept
        requires std::move_constructible<T>
        : value_(std::move(strong.value_)) {}
};

template <typename... Skills>
struct skills {};

namespace ops {
    struct access {};
    struct addition {};
    struct scaling {};
    struct equality {};
    struct ordering {};
    struct printing {};
    struct hashing {};
} // namespace ops


struct readable {
    using family = ops::access;

    template <typename S>
    struct mixin {
        constexpr const auto& value() const {
            return skill_access::get(static_cast<const S&>(*this));
        }
    };
};


template <auto Op>
struct addable_with {
    using family = ops::addition;

    template <typename T>
    static constexpr bool accepts = requires(const T& a, const T& b) {
        { Op(a, b) } -> std::constructible_from<T>;
    };

    template <typename S>
    struct mixin {
        friend constexpr S operator+(const S& a, const S& b) {
            return S(Op(skill_access::get(a), skill_access::get(b)));
        }

        constexpr S& operator+=(const S& b) {
            auto& self = static_cast<S&>(*this);
            self = self + b;
            return self;
        }
    };
};

using addable = addable_with<std::plus{}>;

template <typename Scalar>
struct scalable {
    using family = ops::scaling;

    template <typename S>
    struct mixin {
        friend constexpr S operator*(const Scalar& a, const S& b) {
            return S(Op(a, skill_access::get(b)));
        }

        friend constexpr S operator*(const S& a, const Scalar& b) {
            return S(Op(skill_access::get(a), b));
        }

        friend constexpr S operator/(const S& a, const Scalar& b) {
            return S(Op(skill_access::get(a), b));
        }
    };
};


template <auto Eq>
struct equality_with {
    using family = ops::equality;
    template <typename S>
    struct mixin {
        friend constexpr bool operator==(const S& a, const S& b) {
            return Eq(skill_access::get(a), skill_access::get(b));
        }

        friend constexpr bool operator!=(const S& a, const S& b) {
            return !Eq(skill_access::get(a), skill_access::get(b));
        }
    };
};

using equality = equality_with<std::equal_to<>{}>;


template <auto Cmp>
struct ordered_with {
    using family = ops::ordering;

    using needs = equality_with<[](const auto& x, const auto& y) {
        return Cmp(x, y) == 0;
    }>;

    template <typename S>
    struct mixin {
        friend constexpr auto operator<=>(const S& a, const S& b) {
            return Cmp(skill_access::get(a), skill_access::get(b));
        }

        friend constexpr bool operator==(const S& a, const S& b) {
            return Cmp(skill_access::get(a), skill_access::get(b)) == 0;
        }

        friend constexpr bool operator!=(const S& a, const S& b) {
            return Cmp(skill_access::get(a), skill_access::get(b)) != 0;
        }
    };
};

using ordered = ordered_with<std::compare_three_way{}>;


template <std::size_t N>
struct FixedString {
    char data[N]{};

    constexpr FixedString(const char (&str)[N]) {
        std::copy_n(str, N, data);
    }
};

template <FixedString suffix = "">
struct printable {
    using family = ops::printing;

    template <typename S>
    struct mixin {
        friend std::ostream& operator<<(std::ostream& os, const S& s) {
            return os << skill_access::get(s) << suffix;
        }
    };
};


template <auto H>
struct hashable_with {
    using family = ops::hashing;

    template <typename S>
    struct mixin {};
};

template <typename S>
using hashable = hashable_with<std::hash<S>{}>;


template <typename Target, auto Conv>
struct convertible_to {
    template <typename S>
    struct mixin {};
};


template <typename Other, typename Result, auto Op = std::divides{}>
struct divisible_by {
    template <typename S>
    struct mixin {};
};

#endif // STRONG_STRONG_H
