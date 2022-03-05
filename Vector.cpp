#include <iostream>
#include "Allocator.h"
#include "Algo.h"
using namespace std;

// 和标准的vector相比，还少关于Vector的一些构造函数方面的东西。
// 其余的都基本写好。 


template <class T, class Alloc = alloc>
class Vector{
	public:
		typedef T value_type;
		typedef value_type* pointer;
		typedef value_type& reference;
		typedef value_type* iterator;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
	
	protected:
		typedef simple_alloc<value_type, Alloc> data_allocator;
		
		iterator start;
		iterator finish;
		iterator end_of_storage;

		void insert_aux(iterator position, const value_type& x);
		
	public:
		Vector():start(0), finish(0), end_of_storage(0){	}
		
		Vector(size_type n, const value_type& value){	fill_initialize(n, value);	}
		
		template<class Iterator>
		Vector(Iterator first, Iterator last):start(0), finish(0), end_of_storage(0){
			range_initialize(first, last);
		} 
		
		~Vector(){	destroy(start, finish);		deallocate();	}
		
		explicit Vector(size_type n){	fill_initialize(n, value_type());	}
	public:
		iterator begin(){	return start;	}
		iterator end(){	return finish;	}
		size_type capacity(){	return size_type(end_of_storage - begin());	}
		bool empty() const {
			return end()==begin();
		}
		reference operator[](size_type n){
			return *(begin() + n);
		}
		size_type size(){
			return size_type(end() - begin());
		}
		
		void push_back(const value_type& x){
			if(finish!=end_of_storage){
				construct(finish, x);
				++finish;
			}else
				insert_aux(end(), x);
		}
		void pop_back(){
			--finish;
			destroy(finish);  
		}
		
		void Swap(Vector<T, Alloc>& x){
			swap(start, x.start);
			swap(finish, x.finish);
			swap(end_of_storage, x.end_of_storage);
		}
		
		iterator erase(iterator position){
			if(position + 1 != end())
				copy(position+1, finish, position);
			--finish;
			destroy(finish);
			return position;
		}
		iterator erase(iterator first, iterator last){
			iterator i = Copy(last, finish, first);
			destroy(i, finish);
			finish = finish -  (last - first);
			return first;
		}
		
		void resize(size_type new_size, const T& x){
			if(new_size > size())
				insert(finish, new_size - size(), x);
			else
				erase(begin() + new_size, end());
		}
		void resize(size_type new_size){	resize(new_size, value_type());	}
		
		void reserve(size_type n){
			if(capacity() < n){
				const size_type old_size = size();
				iterator tmp = allocate_and_copy(n, start, finish);
				destroy(start, finish);
				start = tmp;
				finish = start + old_size;
				end_of_storage = start + n;
				cout << "reserve function!!";
			}
		}
		
		iterator insert(iterator position, const value_type& x){
			size_type n = position - begin();
			if(position == end() && finish!=end_of_storage){
//				construcr(position, x);
//				++finish;
				push_back(x);
			}else
				insert_aux(position, x);
			return begin() + n;
		}
		iterator insert(iterator position){	return insert(position, value_type());	}
		void insert(iterator position, size_type n, const value_type& x);
		
	protected:
		void deallocate(){	if(start) data_allocator::deallocate(start, end_of_storage - start);	}
		
		void fill_initialize(size_type n, const value_type& value){
			start = allocate_and_fill(n, value);
			finish = start + n;
			end_of_storage = finish;
		}
		
		iterator allocate_and_fill(size_type n, const value_type& x){
			iterator result = data_allocator::allocate(n);
			uninitialized_fill(result, n, x);
			return result;
		}
		
		iterator allocate_and_copy(size_type n, iterator first, iterator last){
			iterator result = data_allocator::allocate(n);
			uninitialized_copy(first, last, result);
			return result;
		}
		
		template<class Iterator>
		void range_initialize(Iterator first, Iterator last){
			for(; first!=last; ++first)
				push_back(*first);
		}
		
}; 

template <class T, class Alloc>
void Vector<T, Alloc>::insert_aux(iterator position, const value_type& x){
	if(position!=end_of_storage){
		construct(finish, *(finish - 1));
		++finish;
		T x_copy = x;
		copy_backward(position, finish-2, finish -1);
		*position = x_copy;                                          //       ???? position 指向哪？ 
	}else{
		const size_type old_size = size();
		const size_type len = old_size!=0 ? 2*old_size : 1;
		
		iterator new_start = data_allocator::allocate(len);
		iterator new_finish = new_start;
		
		new_finish = uninitialized_copy(start, position, new_start);
		construct(new_finish, x);
		++new_finish;
		new_finish = uninitialized_copy(position, finish, new_finish);
		
		destroy(begin(), end());
		deallocate();
		start = new_start;
		finish = new_finish;
		end_of_storage = start + len;
	}
}

template <class T, class Alloc>
void Vector<T, Alloc>::insert(iterator position, size_type n, const value_type& x){
	if(n!=0){
		if(size_type(end_of_storage - finish) >= n){
			value_type x_copy = x;
			const size_type elems_after = finish - position;
			iterator old_finish = finish;
			
			if(elems_after >= n){
				uninitialized_copy(finish - n, finish, finish);
				finish += n;
				Copy_backward(position, old_finish - n, old_finish);
				fill(position, position + n, x_copy);
			}else{
				uninitialized_fill_n(finish, n - elems_after, x_copy);
				finish += n - elems_after;
				uninitialized_copy(position, old_finish, finish);
				finish += elems_after;
				fill(position, old_finish, x_copy);
			}
		}else{
			const size_type old_size = size();
			const size_type len = old_size + max(old_size, n);
			
			iterator new_start = data_allocator::allocate(len);
			iterator new_finish = new_start;
			
			new_finish = uninitialized_copy(start, position, new_start); 
			new_finish = uninitialized_fill_n(new_finish, n, x);
			new_finish = uninitialized_copy(position, finish, new_finish); 
			
			destroy(start, finish);
			deallocate();
			start = new_start;
			finish = new_finish;
			end_of_storage = start + len; 
		}
	}
}

/*                                    test function                                          */
namespace lw00{
	
	void test00(){
		Vector<int> v;
		
		for(int i=0; i<10; ++i)
			v.push_back(i);

		for(int i=0; i<v.size(); ++i){	
			cout << v[i] << endl;
		}
		
		cout << "size : " << v.size() << endl;
		cout << "----------------------------------" << endl;
		

		for(int i=0; i<v.size(); ++i){	
			cout << v[i] << endl;
		}
		cout << "pop_back()  size : " << v.size() << endl;
		cout << "----------------------------------" << endl;
		cout << "this is my first Vector! " << endl;
	}
	
	void test01(){
		Vector<int> v;
		
		for(int i=0; i<10; ++i)
			v.push_back(i);

		for(int i=0; i<v.size(); ++i){	
			cout << v[i] << endl;  // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
		}
		
		cout << "size : " << v.size() << endl;
		cout << "----------------------------------" << endl;
		
		Vector<int>::iterator it = v.begin();
		v.erase(it + 3, it + 6);   

		for(int i=0; i<v.size(); ++i){	
			cout << v[i] << endl;    // 0, 1, 2, 6, 7, 8, 9
		}
		cout << "erase()  size : " << v.size() << endl;
		cout << "----------------------------------" << endl;
		cout << "this is my first Vector! " << endl;
		
		cout << "capacity : " << v.capacity() << endl;
		cout << "size : " << v.size() << endl;
 	}
	
	void test02(){
		Vector<int> v;
		
		for(int i=0; i<10; ++i)
			v.push_back(i);

		for(int i=0; i<v.size(); ++i){	
			cout << v[i] << endl;  // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
		}
		
		cout << "size : " << v.size() << endl;
		cout << "----------------------------------" << endl;
		
		v.reserve(100);
	}
	
	void test03(){ // OK
		Vector<int> v;
		
		for(int i=0; i<10; ++i)
			v.push_back(i);

		for(int i=0; i<v.size(); ++i){	
			cout << v[i] << endl;  // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
		}
		
		v.resize(5);
		cout << "v.resize(5) :  " << endl;
		cout << "--------------------------" <<endl;
		for(int i=0; i<v.size(); ++i){	
			cout << v[i] << endl;  // 0, 1, 2, 3, 4
		}
		
		v.resize(15);
		cout << "v.resize(15) :  " << endl;
		cout << "--------------------------" <<endl;
		for(int i=0; i<v.size(); ++i){	
			cout << v[i] << endl;  // 0, 1, 2, 3, 4, 0*10
		}
	}
	
	void test04(){
		Vector<int> v;
		
		for(int i=0; i<10; ++i)
			v.push_back(i);

		for(int i=0; i<v.size(); ++i){	
			cout << v[i] << endl;  // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
		}
		
		Vector<int>::iterator it = v.begin();
		
		Vector<int> v1(it + 3, it + 7);
		
		cout << "--------------------" << endl;
		cout << endl;
		cout << "v1.size()  " << v1.size() << endl;
		for(int i=0; i<v1.size(); ++i){	
			cout << v1[i] << endl;  // 3, 4, 5, 6
		}
	}
}

int main(){
//	lw00::test00();
//	lw00::test01();
//	lw00::test02();
//	lw00::test03();
	lw00::test04();
	return 0;
}
