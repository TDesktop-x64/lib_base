// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/basic_types.h"

#include <QObject>
#include <QPointer>

#include <atomic>
#include <memory>

namespace base {

class has_weak_ptr;

template <typename T>
class weak_qptr {
public:
	weak_qptr() = default;
	weak_qptr(std::nullptr_t) noexcept {
	}
	weak_qptr(T *value) noexcept
	: _object(static_cast<const QObject*>(value)) {
	}
	weak_qptr(const QPointer<T> &value) noexcept
	: weak_qptr(value.data()) {
	}
	weak_qptr(gsl::not_null<T*> value) noexcept
	: weak_qptr(value.get()) {
	}
	weak_qptr(const std::unique_ptr<T> &value) noexcept
	: weak_qptr(value.get()) {
	}
	weak_qptr(const std::shared_ptr<T> &value) noexcept
	: weak_qptr(value.get()) {
	}
	weak_qptr(const std::weak_ptr<T> &value) noexcept
	: weak_qptr(value.lock().get()) {
	}
	weak_qptr(const weak_qptr &other) noexcept
	: _object(other._object) {
	}
	weak_qptr(weak_qptr &&other) noexcept
	: _object(std::move(other._object)) {
	}
	template <
		typename Other,
		typename = std::enable_if_t<
			std::is_base_of_v<T, Other> && !std::is_same_v<T, Other>>>
	weak_qptr(const weak_qptr<Other> &other) noexcept
	: _object(other._object) {
	}
	template <
		typename Other,
		typename = std::enable_if_t<
			std::is_base_of_v<T, Other> && !std::is_same_v<T, Other>>>
	weak_qptr(weak_qptr<Other> &&other) noexcept
	: _object(std::move(other._object)) {
	}

	weak_qptr &operator=(T *value) noexcept {
		reset(value);
		return *this;
	}
	weak_qptr &operator=(const QPointer<T> &value) noexcept {
		reset(value.data());
		return *this;
	}
	weak_qptr &operator=(gsl::not_null<T*> value) noexcept {
		reset(value.get());
		return *this;
	}
	weak_qptr &operator=(const std::unique_ptr<T> &value) noexcept {
		reset(value.get());
		return *this;
	}
	weak_qptr &operator=(const std::shared_ptr<T> &value) noexcept {
		reset(value.get());
		return *this;
	}
	weak_qptr &operator=(const std::weak_ptr<T> &value) noexcept {
		reset(value.lock().get());
		return *this;
	}
	weak_qptr &operator=(const weak_qptr &other) noexcept {
		_object = other._object;
		return *this;
	}
	weak_qptr &operator=(weak_qptr &&other) noexcept {
		_object = std::move(other._object);
		return *this;
	}
	template <
		typename Other,
		typename = std::enable_if_t<
			std::is_base_of_v<T, Other> && !std::is_same_v<T, Other>>>
	weak_qptr &operator=(const weak_qptr<Other> &other) noexcept {
		_object = other._object;
		return *this;
	}
	template <
		typename Other,
		typename = std::enable_if_t<
			std::is_base_of_v<T, Other> && !std::is_same_v<T, Other>>>
	weak_qptr &operator=(weak_qptr<Other> &&other) noexcept {
		_object = std::move(other._object);
		return *this;
	}

	[[nodiscard]] bool null() const noexcept {
		return _object.isNull();
	}
	[[nodiscard]] bool empty() const noexcept {
		return !_object;
	}
	[[nodiscard]] T *get() const noexcept {
		const auto strong = _object.data();
		if constexpr (std::is_const_v<T>) {
			return static_cast<T*>(strong);
		} else {
			return const_cast<T*>(static_cast<const T*>(strong));
		}
	}
	[[nodiscard]] explicit operator bool() const noexcept {
		return !empty();
	}
	[[nodiscard]] T &operator*() const noexcept {
		return *get();
	}
	[[nodiscard]] T *operator->() const noexcept {
		return get();
	}

	friend inline auto operator<=>(
		weak_qptr,
		weak_qptr) noexcept = default;

	void reset(T *value = nullptr) noexcept {
		_object = static_cast<const QObject*>(value);
	}

private:
	QPointer<const QObject> _object;

};

template <typename T>
inline bool operator==(const weak_qptr<T> &pointer, std::nullptr_t) noexcept {
	return (pointer.get() == nullptr);
}

template <typename T>
inline bool operator==(std::nullptr_t, const weak_qptr<T> &pointer) noexcept {
	return (pointer == nullptr);
}

template <typename T>
inline bool operator!=(const weak_qptr<T> &pointer, std::nullptr_t) noexcept {
	return !(pointer == nullptr);
}

template <typename T>
inline bool operator!=(std::nullptr_t, const weak_qptr<T> &pointer) noexcept {
	return !(pointer == nullptr);
}

template <
	typename T,
	typename = std::enable_if_t<std::is_base_of_v<QObject, T>
		&& !std::is_base_of_v<has_weak_ptr, T>>>
weak_qptr<T> make_weak(T *value) {
	return value;
}

template <
	typename T,
	typename = std::enable_if_t<std::is_base_of_v<QObject, T>
		&& !std::is_base_of_v<has_weak_ptr, T>>>
weak_qptr<T> make_weak(gsl::not_null<T*> value) {
	return value;
}

template <
	typename T,
	typename = std::enable_if_t<std::is_base_of_v<QObject, T>
		&& !std::is_base_of_v<has_weak_ptr, T>>>
weak_qptr<T> make_weak(const std::unique_ptr<T> &value) {
	return value;
}

template <
	typename T,
	typename = std::enable_if_t<std::is_base_of_v<QObject, T>
		&& !std::is_base_of_v<has_weak_ptr, T>>>
weak_qptr<T> make_weak(const std::shared_ptr<T> &value) {
	return value;
}

template <
	typename T,
	typename = std::enable_if_t<std::is_base_of_v<QObject, T>
		&& !std::is_base_of_v<has_weak_ptr, T>>>
weak_qptr<T> make_weak(const std::weak_ptr<T> &value) {
	return value;
}

} // namespace base

namespace crl {

template <typename T, typename Enable>
struct guard_traits;

template <typename T>
struct guard_traits<base::weak_qptr<T>, void> {
	static base::weak_qptr<T> create(const base::weak_qptr<T> &value) {
		return value;
	}
	static base::weak_qptr<T> create(base::weak_qptr<T> &&value) {
		return std::move(value);
	}
	static bool check(const base::weak_qptr<T> &guard) {
		return guard.get() != nullptr;
	}

};

} // namespace crl
