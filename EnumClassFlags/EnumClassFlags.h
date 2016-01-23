#ifndef __INCLUDED_ENUMCLASSFLAGS_H
#define __INCLUDED_ENUMCLASSFLAGS_H
#include <type_traits>

template <typename T>
struct is_flag_enum
{
	static const bool value = false;
};

#define DEFINE_FLAG_ENUM_CLASS(name) \
enum class name; \
template <>\
struct is_flag_enum<name>\
{\
static const bool value = true;\
};\
enum class name

#define DECLARE_FLAG_CLASS(name)\
template <>\
struct is_flag_enum<name>\
{\
static const bool value = true;\
};\

template <int size>
struct integral_by_size
{
	static_assert(size > sizeof(size_t), "Size must be smaller than sizeof(size_t).");
	typedef size_t type;
};

template <>
struct integral_by_size<1>
{
	typedef unsigned char type;
};

template <>
struct integral_by_size<2>
{
	typedef unsigned short type;
};

template <>
struct integral_by_size<4>
{
	typedef unsigned int type;
};

template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, T>::type operator~ (T a) { return (T)~(integral_by_size<sizeof(T)>::type)a; }
template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, T>::type operator| (T a, T b) { return (T)((integral_by_size<sizeof(T)>::type)a | (integral_by_size<sizeof(T)>::type)b); }
template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, T>::type operator& (T a, T b) { return (T)((integral_by_size<sizeof(T)>::type)a & (integral_by_size<sizeof(T)>::type)b); }
template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, T>::type operator^ (T a, T b) { return (T)((integral_by_size<sizeof(T)>::type)a ^ (integral_by_size<sizeof(T)>::type)b); }
template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, T>::type& operator|= (T& a, T b) { return (T&)((integral_by_size<sizeof(T)>::type&)a |= (integral_by_size<sizeof(T)>::type)b); }
template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, T>::type& operator&= (T& a, T b) { return (T&)((integral_by_size<sizeof(T)>::type&)a &= (integral_by_size<sizeof(T)>::type)b); }
template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, T>::type& operator^= (T& a, T b) { return (T&)((integral_by_size<sizeof(T)>::type&)a ^= (integral_by_size<sizeof(T)>::type)b); }

#ifdef _USE_FLAG_ENUM_CLASS_FUNCTIONS
template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, bool>::type TestFlag(T a, T b) { return ((integral_by_size<sizeof(T)>::type)(a&b) != 0); }
template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, T>::type& SetFlag(T& a, T b) { return (a |= b);}
template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, T>::type& ClearFlag(T& a, T b) { return (a &= ~b); }
template<typename T> inline typename std::enable_if<is_flag_enum<T>::value, T>::type& FlipFlag(T&a, T b) { return (a ^= b); }
#endif // _USE_FLAG_ENUM_CLASS_FUNCTIONS

#endif //__INCLUDED_ENUMCLASSFLAGS_H