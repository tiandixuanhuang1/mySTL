#include <iostream>

template<class T1, class T2>
void construct(T1*p, const T2& value){
	new(p) T1(value);
}

template<class T>
void destroy(T* pointer){
	pointer->~T();
}

template<class Iterator>
void destroy(Iterator first, Iterator last){ // [first, last) 
	for(; first < last; ++first)
		destroy(&*first);
}

template<class T>
void Copy(T** first, T** last, T** result){
	for(ptrdiff_t n = last - first; n>0; --n, ++first, ++result) //  [first, last) copy 到result之后，包括result。 
		*result = *first;
}

template<class T>
T** Copy_backward(T** first, T** last, T** result){ // 将 [first, last) copy 到 result 之前（不包括 result）。 
	while(first != last){
		*--result = *--last;
	}
	return result;
}

template <class Iterator>
Iterator Copy(Iterator first, Iterator last, Iterator result){
	for(ptrdiff_t n = last - first; n>0; --n, ++first, ++result)
		*result = *first;
	return result;
} 

template <class Iterator>
void Copy_backward(Iterator first, Iterator last, Iterator result){
	while(first!=last)
		*--result = *--last;
}


template <class Iterator, class value_type>
void uninitialized_fill(Iterator first, Iterator last, const value_type& x){
	Iterator cur_iter = first;
	for( ; cur_iter!=last; ++cur_iter)
		construct(&*cur_iter, x);                     // *cur_iter 为 cur 所指的值，&(*cur_iter) 就是指向值的地址。 
}

template <class Iterator>
Iterator uninitialized_copy(Iterator first, Iterator last, Iterator result){
	for(; first != last; ++first, ++result)
		construct(&*result, *first);
	return result;
}

template<class Iterator, class T>
void __uninitialized_copy_fill(Iterator first1, Iterator last1, Iterator first2, Iterator last2, const T& x){
	Iterator mid2 = uninitialized_copy(first1, last1, first2);
	uninitialized_fill(mid2, last2, x);
}

template<class Iterator, class T>
Iterator __uninitialized_fill_copy(Iterator result, Iterator mid, const T& x, Iterator first, Iterator last){
	uninitialized_fill(result, mid, x);
	return uninitialized_copy(first, last, mid);
}

template<class Iterator, class T>
Iterator uninitialized_fill_n(Iterator position, size_t n, const T& x){
	for(; n>0; --n, ++position)
		construct(&*position, x);
	return position;
}
