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

template <strong_details::strong_value T, typename Tag, typename... Skills>
class strong;

namespace strong_check {
    template <typename... S>
    struct is_strong : std::false_type {};

    template <typename... S>
    struct is_strong<strong<S...>> : std::true_type {};
}

template <typename S>
concept is_strong_v = strong_check::is_strong<std::decay<S>>::value;

template <typename S>
using underlying_t = S::value_type;

// TODO has_family_v<S, F>

template <typename... PackSkills>
struct skills {
    /** A base struct for the `strong` class to inherit skills from.
     *  Only works if `Skills` have been flattened. */
    template <typename T, typename Tag, typename... Skills>
    struct mixin_base
        : PackSkills::template mixin<strong<T, Tag, Skills...>>... {};
};

// TODO: merge namespaces
namespace strong_details {
    template <typename T, typename... S>
    struct recursive_flatten {};

    template <typename... CurrentSkills>
    struct recursive_flatten<skills<CurrentSkills...>> {
        using type = skills<CurrentSkills...>;
    };

    template <typename... CurrentSkills, typename Skill,
              typename... OtherSkills>
    struct recursive_flatten<skills<CurrentSkills...>, Skill, OtherSkills...> {
        // Move `Skills` to the output list and queue the remaining
        // skills and skill packs for processing.
        using type = recursive_flatten<skills<CurrentSkills..., Skill>,
                                       OtherSkills...>::type;
    };

    template <typename... CurrentSkills, typename... PackedSkills,
              typename... OtherSkills>
    struct recursive_flatten<skills<CurrentSkills...>, skills<PackedSkills...>,
                             OtherSkills...> {
        // Unpack `PackedSkills` and queue them for processing together with the
        // remaining skills and skill packs.
        using type = recursive_flatten<skills<CurrentSkills...>,
                                       PackedSkills..., OtherSkills...>::type;
    };

    template <typename... Skills>
    struct flatten {
        using type = recursive_flatten<skills<>, Skills...>::type;
    };

    /** Flattens the provided skill list.
     *  Preserves the order in which skills appeared on the non-flattened list. */
    template <typename... Skills>
    using flatten_t = flatten<Skills...>::type;
} // namespace strong_details

class skill_access {
public:
    // TODO: shouldn't compile when S isn't instance of strong
    // Does this work? If not, `requires` should
    template <is_strong_v S>
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
class strong : public strong_details::flatten_t<skills<Skills...>>
                                    ::mixin_base<T, Tag, Skills...> {
    T value_;

public:
    friend class skill_access;

    using value_type = T;
    using tag_type = Tag;

    constexpr strong()
        requires std::default_initializable<T>
        : value_() {}

    template <typename... Args>
    constexpr explicit strong(Args&&... args)
        : value_(std::forward<Args>(args)...) {}

    constexpr strong(strong& strong)
        requires std::copy_constructible<T>
        : value_(strong.value_) {}

    constexpr strong(strong&& strong) noexcept
        requires std::move_constructible<T>
        : value_(std::move(strong.value_)) {}

    constexpr strong& operator=(const strong& other)
        requires std::copyable<T> {
        value_ = other.value_;
        return *this;
    }

    constexpr strong& operator=(strong&& other) noexcept
        requires std::movable<T> {
        value_ = std::move(other.value_);
        return *this;
    }
};

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
    static constexpr bool accepts = requires(const T& a, const T& b)
    {
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

    template <typename T>
    static constexpr bool accepts = requires(const Scalar& a, const T& b)
    {
        { Op(a, b) } -> std::constructible_from<T>;
    };

    template <typename S>
    struct mixin {
        friend constexpr S operator*(const S& a, const Scalar& s) {
            return S(skill_access::get(a) * s);
        }

        friend constexpr S operator*(const Scalar& s, const S& a) {
            return S(s * skill_access::get(a));
        }

        friend constexpr S operator/(const S& a, const Scalar& s) {
            return S(skill_access::get(a) / s);
        }

        constexpr S& operator*=(const Scalar& s) {
            auto& self = static_cast<S&>(*this);
            self = self * s;
            return self;
        }

        // TODO: *= the other way?

        constexpr S& operator/=(const Scalar& s) {
            auto& self = static_cast<S&>(*this);
            self = self / s;
            return self;
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
    };
};

using equality = equality_with<std::equal_to{}>;


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
    };
};

using ordered = ordered_with<std::compare_three_way{}>;


namespace str_literal {
    template <std::size_t N>
    struct FixedString {
        char data[N]{};

        constexpr FixedString(const char (&str)[N]) {
            std::copy_n(str, N, data);
        }
    };
} // namespace str_literal

template <str_literal::FixedString suffix = "">
struct printable {
    using family = ops::printing;

    template <typename S>
    struct mixin {
        friend std::ostream& operator<<(std::ostream& os, const S& s) {
            return os << skill_access::get(s) << suffix.data;
        }
    };
};


template <auto H>
struct hashable_with {
    using family = ops::hashing;

    // TODO: check if correct
    using needs = equality;

    template <typename S>
    struct mixin {
        constexpr auto operator()(const S& s) const {
            return H(skill_access::get(s));
        }
    };
};

template <typename S>
using hashable = hashable_with<std::hash<S>{}>;


template <typename Target, auto Conv = std::identity{}>
struct convertible_to {
    template <typename S>
    struct mixin {
        constexpr explicit operator Target() {
            return Target(Conv(static_cast<S&>(*this)));
        }
    };
};


template <typename Other, typename Result, auto Op = std::divides{}>
struct divisible_by {
    template <typename S>
    struct mixin {
        constexpr friend Result operator/(const S& a, const Other& other) {
            return Op(skill_access::get(a), other);
        }

        constexpr friend Result operator/(const S& a, const S& b) {
            return Op(skill_access::get(a), skill_access::get(b));
        }
    };
};
#endif // STRONG_STRONG_H
