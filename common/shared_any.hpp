// Copyright (c) 2013-2016 Trillek contributors. See AUTHORS.txt for details
// Licensed under the terms of the LGPLv3. See licenses/lgpl-3.0.txt

#ifndef TRILLEK_COMMON_SHARED_ANY_HPP
#define TRILLEK_COMMON_SHARED_ANY_HPP

#include <memory>
#include <typeinfo>
#include <type_traits>
#include <any>

// A non copyable variant of util::any.
// The two main use cases for such a container are
//  1. storing types that are not copyable.
//  2. ensuring that no copies are made of copyable types that have to be stored
//     in a type-erased container.
//
// unique_any has the same semantics as any with the execption of copy and copy
// assignment, which are explicitly forbidden for all contained types.
// The requirement that the contained type be copy constructable has also been
// relaxed.
//
// The any_cast non-member functions have been overridden for unique_any, with
// the same semantics as for any.
// This makes it possible to copy the underlying stored type if the type is
// copyable. For example, the following code will compile and execute as
// expected.
//
//  unique_any<int> a(3);
//  int& ref = any_cast<int&>(a); // take a reference
//  ref = 42;                     // update contained value via reference
//  int  val = any_cast<int>(a);  // take a copy
//  assert(val==42);
//
// If the underlying type is not copyable, only references may be taken
//
//  unique_any<nocopy_t> a();
//  nocopy_t& ref        = any_cast<nocopy_t&>(a);       // ok
//  const nocopy_t& cref = any_cast<const nocopy_t&>(a); // ok
//  nocopy_t v           = any_cast<nocopy_t>(a);        // compile time error
//
// An lvalue can be created by moving from the contained object:
//
//  nocopy_t v = any_cast<nocopy_t&&>(std::move(a)); // ok
//
// After which a is in moved from state.

namespace tec {

namespace detail {
template <typename T>
using any_cast_remove_qual = std::remove_cv_t<std::remove_reference_t<T>>; 
}

class shared_any final {
public:
	constexpr shared_any() = default;

	shared_any(shared_any const& other) = default;

	shared_any(shared_any&& other) noexcept {
		swap(other);
	}

	template <
		typename T,
		typename = std::enable_if_t<!std::is_same<std::decay_t<T>, shared_any>::value>
	>
	shared_any(T&& other) {
		_state.reset(new model<contained_type<T>>(std::forward<T>(other)));
	}

	shared_any& operator=(shared_any const& other) = default;
	shared_any& operator=(shared_any&& other) noexcept {
		swap(other);
		return *this;
	}

	template <
		typename T,
		typename = std::enable_if_t<!std::is_same<std::decay_t<T>, shared_any>::value>
	>
	shared_any& operator=(T&& other) {
		_state.reset(new model<contained_type<T>>(std::forward<T>(other)));
		return *this;
	}

	void reset() noexcept {
		_state.reset();
	}

	void swap(shared_any& other) noexcept {
		std::swap(other._state, _state);
	}

	bool has_value() const noexcept {
		return (bool)_state;
	}

	const std::type_info& type() const noexcept {
		return has_value()? _state->type(): typeid(void);
	}

private:
	template <typename T>
	using contained_type = std::decay_t<T>;

	struct interface {
		virtual ~interface() = default;
		virtual const std::type_info& type() = 0;
		virtual void* pointer() = 0;
		virtual const void* pointer() const = 0;
		[[noreturn]] virtual void throw_pointer() = 0;
	};

	template <typename T>
	struct model : public interface {
		~model() = default;
		model(const T& other): value(other) {}
		model(T&& other): value(std::move(other)) {}

		const std::type_info& type() override { return typeid(T); }
		[[noreturn]] void throw_pointer() override { throw &value; }
		void* pointer() override { return &value; }
		const void* pointer() const override { return &value; }

		T value;
	};

	std::shared_ptr<interface> _state;

protected:
	template <typename T>
	friend T const* any_cast(const shared_any* operand);

	template <typename T>
	friend T* any_cast(shared_any* operand);

	template <typename T>
	friend std::shared_ptr<T const> shared_any_cast(shared_any const& operand);

	template <typename T>
	friend std::shared_ptr<T> shared_any_cast(shared_any& operand);

	template <typename U>
	friend U* any_base_cast(shared_any* operand);

	template <typename U>
	friend std::shared_ptr<U> shared_any_base_cast(shared_any& operand);

	void throw_ptr() {
		return _state->throw_pointer();
	}

	template <typename T>
	T* unsafe_cast() {
		return static_cast<T*>(_state->pointer());
	}

	template <typename T>
	T const* unsafe_cast() const {
		return static_cast<T const*>(_state->pointer());
	}
	
	template <typename T>
	std::shared_ptr<T> unsafe_shared_cast() {
		auto mtptr = std::static_pointer_cast<model<T>>(_state);
		std::shared_ptr<T> tptr{mtptr, &mtptr->value};
		return tptr;
	}

	template <typename T>
	std::shared_ptr<T const> unsafe_shared_cast() const {
		auto mtptr = std::static_pointer_cast<model<T const>>(_state);
		std::shared_ptr<T const> tptr{mtptr, &mtptr->value};
		return tptr;
	}
};

// If operand is not a null pointer, and the typeid of the requested T matches
// that of the contents of operand, a pointer to the value contained by operand,
// otherwise a null pointer.
template<class T>
const T* any_cast(const shared_any* operand) {
	if (operand && operand->type() == typeid(T)) {
		return operand->unsafe_cast<T>();
	}
	return nullptr;
}

// If operand is not a null pointer, and the typeid of the requested T matches
// that of the contents of operand, a pointer to the value contained by operand,
// otherwise a null pointer.
template<class T>
T* any_cast(shared_any* operand) {
	if (operand && operand->type() == typeid(T)) {
		return operand->unsafe_cast<T>();
	}
	return nullptr;
}

template <typename U>
U* any_base_cast(shared_any *operand) {
	U* uptr = any_cast<U>(operand);
	if (uptr != nullptr)
		return uptr;
	try {
		operand->throw_ptr();
	} catch (U* to_ptr) {
		return to_ptr;
	} catch (...) {
		return nullptr;
	}

	/* this is unreachable but otherwise the compiler will complain */
	return nullptr;
}

template<class U>
std::shared_ptr<U> shared_any_base_cast(shared_any& operand) {
	U* base_ptr = any_base_cast<U>(&operand);
	if (base_ptr == nullptr) {
		throw std::bad_any_cast();
	}

	return operand.unsafe_shared_cast<U>();
}

template<class T>
std::shared_ptr<T> shared_any_cast(shared_any& operand) {
	if (operand.type() == typeid(T)) {
		return operand.unsafe_shared_cast<T>();
	}
	throw std::bad_any_cast();
}

template<class T>
std::shared_ptr<T const> shared_any_cast(shared_any const& operand) {
	if (operand.type() == typeid(T)) {
		return operand.unsafe_shared_cast<T const>();
	}
	throw std::bad_any_cast();
}

template<class T>
T any_cast(const shared_any& operand) {
	using U = detail::any_cast_remove_qual<T>;
	static_assert(std::is_constructible<T, const U&>::value,
			"any_cast type can't construct copy of contained object");

	auto ptr = any_cast<U>(&operand);
	if (ptr==nullptr) {
		throw std::bad_any_cast();
	}
	return static_cast<T>(*ptr);
}

template<class T>
T any_cast(shared_any& operand) {
	using U = detail::any_cast_remove_qual<T>;
	static_assert(std::is_constructible<T, U&>::value,
			"any_cast type can't construct copy of contained object");

	auto ptr = any_cast<U>(&operand);
	if (ptr==nullptr) {
		throw std::bad_any_cast();
	}
	return static_cast<T>(*ptr);
}

template<class T>
T any_cast(shared_any&& operand) {
	using U = detail::any_cast_remove_qual<T>;

	static_assert(std::is_constructible<T, U&&>::value,
			"any_cast type can't construct copy of contained object");

	auto ptr = any_cast<U>(&operand);
	if (ptr==nullptr) {
		throw std::bad_any_cast();
	}
	return static_cast<T>(std::move(*ptr));
}

template <class T, class... Args>
shared_any make_shared_any(Args&&... args) {
	return shared_any(T(std::forward<Args>(args) ...));
}

}

#endif
