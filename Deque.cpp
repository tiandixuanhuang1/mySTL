#include <iostream>
#include <string.h>
#include "Allocator.h"
#include "Algo.h"
using namespace std;


inline size_t __deque_buf_size(size_t n, size_t sz){
	return n!=0?n:(sz<512?(512/sz):size_t(1));
}

/*                                Deque iterator and Deque                                      */
template<class T, size_t BufSiz>
struct __deque_iterator{
	typedef __deque_iterator<T, BufSiz>  iterator;
	static size_t buffer_size(){
		return __deque_buf_size(BufSiz, sizeof(T));
	}
	
	typedef T value_type;
	typedef T* pointer;
	typedef T& reference;
	typedef ptrdiff_t difference_type;
	typedef size_t size_type;
	
	typedef T** map_pointer;
	typedef __deque_iterator self;
	
	T* cur;
	T* first;
	T* last;
	map_pointer node;
	
	__deque_iterator():cur(0), first(0), last(0), node(0){	}
	__deque_iterator(T* x, map_pointer y):cur(x), first(*y), last(*y + buffer_size()), node(y){	}
	__deque_iterator(const self& x):cur(x.cur), first(x.first), last(x.last), node(x.node){	}
	
	void set_node(map_pointer new_node){
		node = new_node;
		first = *new_node;
		last = first + difference_type(buffer_size());
	}

	reference operator*(){	return *cur;	}
	difference_type operator-(const self& x){
		return difference_type(buffer_size())*(node - x.node - 1) + (cur - first) + (x.last - x.cur);
	}
	
	bool operator==(const self& x){	return cur==x.cur;	}
	bool operator!=(const self& x){	return !(cur==x.cur);	}
	bool operator<(const self& x){	return (node == x.node) ? (cur<x.cur) : (node < x.node);	}
	
	self& operator+=(difference_type n){
		difference_type offset = n + (cur - first);
		if(offset < difference_type(buffer_size()) )	cur += n;
		else{
			difference_type node_offset = offset / difference_type(buffer_size());
			set_node(node + node_offset);
			cur = first + (offset - node_offset*difference_type(buffer_size()));
		}
		return *this;
	}
	self operator+(difference_type n){	self tmp = *this;	return tmp += n;	}
	self& operator-=(difference_type n){	return *this += (-n);	}
	self operator-(difference_type n){	self tmp = *this;	return tmp -= n;	}
	
	self& operator++(){
		++cur;
		if(cur==last){
			set_node(node+1);
			cur = first;
		}
		return *this;
	}
	self operator++(int){
		self tmp = *this;
		++*this;
		return tmp;
	}
	self& operator--(){
		if(cur==first){
			set_node(node-1);
			cur = last;
		}
		--cur;
		return *this;
	}
	self operator--(int){
		self tmp = *this;
		--*this;
		return tmp;
	}
};


template<class T, class Alloc=alloc, size_t BufSiz=0>
class Deque{
	public:
		typedef T value_type;
		typedef value_type* pointer;
		typedef value_type& reference;
		typedef size_t size_type;
		typedef T** map_pointer;
		typedef ptrdiff_t difference_type;
		
		typedef __deque_iterator<T, BufSiz> iterator;
		
	protected:
		typedef simple_alloc<value_type, Alloc> data_allocator;
		typedef simple_alloc<pointer, Alloc> map_allocator;
		
		static size_type buffer_size(){	return __deque_buf_size(BufSiz, sizeof(value_type));	}
		static size_type initial_map_size(){	return 8;	}

	protected:
		iterator start;
		iterator finish;
		
		map_pointer map;
		size_type map_size;
		
	public:
		iterator begin(){	return start;	}
		iterator end(){	return finish;	}
		bool empty(){	return (finish==start);	}
		reference operator[](size_type n){	return *(start + n);	}
		
		reference front(){	return *start;	}
		value_type back(){
			iterator tmp = finish;
			--tmp;
			return *tmp;
		}
		
		size_type size(){	return finish - start;	}
		size_type Map_size(){	return map_size;	}
		size_type get_buffer_size(){	return __deque_buf_size(BufSiz, sizeof(value_type));	}
		
		void push_back(const value_type& x){
			if(finish.cur!=finish.last - 1){
				construct(finish.cur, x);
				++finish.cur;
			}else
				push_back_aux(x);
		}
		
		void push_front(const value_type& x){
			if(start.cur!=start.first){
				construct(start.cur-1, x);
				--start.cur;
			}else{
				push_front_aux(x);
			}
		}
		
		void pop_front(){
			if(start.cur!=start.last-1){
				destroy(start.cur);
				++start.cur;
			}else
				pop_front_aux();
		} 
		
		void pop_back(){
			if(finish.cur != finish.first){
				destroy(finish.cur);
				--finish.cur;
			}else
				pop_back_aux();
		}
		
		void swap(Deque& x){
			swap(start, x.start);
			swap(finish, x.finish);
			swap(map, x.map);
			swap(map_size, x.map_size);
		}
		
		iterator erase(iterator pos){
			iterator next = pos;
			++next;
			difference_type index = pos - start;
			if(index < (size() >> 1)){
				Copy_backward(start, pos, next);
				pop_front();
			}else{
				Copy(next, finish, pos);
				pop_back();
			}
			return start + index;
		}
		
		iterator erase(iterator first, iterator last);
		
		iterator insert(iterator position, const value_type& x){
			if(position.cur == start.cur){
				push_front(x);
				return start;
			}else if(position.cur == finish.cur){
				push_back(x);
				return finish;
			}else{
				return insert_aux(position, x);
			}
		}
		
		iterator insert(iterator position){	return insert(position, value_type());	}
		
		void insert(iterator position, size_type n, const value_type& x);
		
		void clear();
		
		void resize(size_type new_size, const value_type& x){
			const size_type len = size();
			if(new_size < len)
				erase(start+new_size, finish);
			else
				insert(finish, new_size - len, x);
				// insert(,,) ->  
		}

		// resize() -> insert(, , ) -> uninitialized_fill()	-> reserve_elements_at_front() -> new_elements_at_front()
		//                                                 \\-> reserve_elements_at_back()	
		//                                                  \->uninitialized_fill()   
	public:
		Deque():start(), finish(), map(0), map_size(0){	create_map_and_nodes(0);	}
	 	~Deque(){
	 		destroy(start, finish);
	 		destroy_map_and_nodes();
		 }
	 	
	protected:
		void push_back_aux(const value_type& t);
	 	// push_back_aux() -> reserve_map_at_back() -> reallocate_map() -> copy(), copy_backward()
	 	//              \-> allocate_node() 
	 	void push_front_aux(const value_type& t);
	 	
	 	void pop_back_aux();
	 	//pop_back_aux() -> deallocate_node()
	 	//               \-> destroy() 
		 void pop_front_aux();
		 
		 iterator insert_aux(iterator position, const value_type& x);
		 
		 void insert_aux(iterator position, size_type n, const value_type& x);
		 
	private:
		pointer allocator_node(){	return data_allocator::allocate(buffer_size());	}
		void deallocate_node(pointer n){	data_allocator::deallocate(n, buffer_size());	}
		void create_map_and_nodes(size_type num_elements);
		void reallocate_map(size_type node_to_add, bool add_at_front);
		void destroy_map_and_nodes();
		
		void reserve_map_at_back(size_type nodes_to_add = 1){
			if(nodes_to_add + 1 > map_size - (finish.node - map))
				reallocate_map(nodes_to_add, false);
		}
		
		void reserve_map_at_front(size_type nodes_to_add=1){
			if(nodes_to_add + 1 > start.node - map)
				reallocate_map(nodes_to_add, true);
		}
	
	    iterator reserve_elements_at_front(size_type n){
	    	size_type vacancies = start.cur - start.first;
	    	if(n > vacancies)
	    		new_elements_at_front(n-vacancies);
	    	return start - difference_type(n);
		}	
		
		iterator reserve_elements_at_back(size_type n){
			size_type vacancies = finish.last - finish.cur -1;
			if(vacancies < n)
				new_elements_at_back(n-vacancies);
			return finish + difference_type(n);
		} 
		
		void new_elements_at_front(size_type new_elements); //
		
		void new_elements_at_back(size_type new_elements); //
}; 

/*******************************            函数实现           **************************/

	template<class T, class Alloc, size_t BufSize> 
	void Deque<T, Alloc, BufSize>::create_map_and_nodes(size_type num_elements){
		size_type num_nodes = num_elements / buffer_size() + 1;
		map_size = max(initial_map_size(), num_nodes + 2);
		map = map_allocator::allocate(map_size);
		
		map_pointer nstart = map + (map_size - num_nodes)/2;
		map_pointer nfinish = nstart + num_nodes - 1;
		
		map_pointer cur;
		
		// 指针的 ++ 操作默认为 移一个存储单位。 
		for(cur=nstart; cur<=nfinish; ++cur)
			*cur = allocator_node();
		start.set_node(nstart);
		finish.set_node(nfinish);
		start.cur = start.first;
		finish.cur = finish.first + num_elements % buffer_size();                   // %是求余！！！！！！ 
	}
	
	template<class T, class Alloc, size_t BufSize>
	void Deque<T, Alloc, BufSize>::reallocate_map(size_type nodes_to_add, bool add_at_front){
		size_type old_num_nodes = finish.node - start.node + 1;
		size_type new_num_nodes = old_num_nodes + nodes_to_add;
		
		map_pointer new_nstart;
		if(map_size>2*new_num_nodes){
			new_nstart = map + (map_size - new_num_nodes)/2 + (add_at_front?nodes_to_add:0);
			if(new_nstart < start.node)
				Copy(start.node, finish.node + 1, new_nstart);
			else
				Copy_backward(start.node, finish.node+1, new_nstart + old_num_nodes); 
		}else{
			size_type new_map_size = map_size + max(map_size, nodes_to_add) + 2;
			
			map_pointer new_map = map_allocator::allocate(new_map_size);
			new_nstart = new_map + (new_map_size - new_num_nodes)/2 
								 + (add_at_front ? nodes_to_add:0);
			Copy(start.node, finish.node+1, new_nstart);
			map_allocator::deallocate(map, map_size);
			
			map = new_map;
			map_size = new_map_size;
		}
		start.set_node(new_nstart); // 确定start迭代器的位置 
		finish.set_node(new_nstart + old_num_nodes - 1); // 确定finish迭代器的位置 
	}

	template<class T, class Alloc, size_t BufSize>
	void Deque<T, Alloc, BufSize>::destroy_map_and_nodes(){
		for(map_pointer cur_node=start.node; cur_node<=finish.node; ++cur_node)
			deallocate_node(*cur_node);
		map_allocator::deallocate(map, map_size);   
	}

	template<class T, class Alloc, size_t BufSize>
	typename Deque<T, Alloc, BufSize>::iterator  Deque<T, Alloc, BufSize>::erase(iterator first, iterator last){
		if(first == start && last == finish){
			clear();
			return finish;
		}else{
			difference_type n = last - first;
			difference_type elems_before = first - start;
			if(elems_before < (size() - n)/2){
				Copy_backward(start, first, last);
				iterator new_start = start + n;
				destroy(start, new_start);
				for(map_pointer cur_node = start.node; cur_node < new_start.node; ++cur_node)
					data_allocator::deallocate(*cur_node, buffer_size());
				start = new_start;
			}else{
				Copy(last, finish, first);
				iterator new_finish = finish - n;
				destroy(new_finish, finish);
				for(map_pointer cur_node = new_finish.node + 1; cur_node <=finish.node; ++cur_node)
					data_allocator::deallocate(*cur_node, buffer_size());
				finish = new_finish;
			}
			return start + elems_before;
		}
	}
	
	template<class T, class Alloc, size_t BufSize>
	void  Deque<T, Alloc, BufSize>::clear(){
		for(map_pointer node = start.node + 1; node < finish.node; ++node){
			destroy(*node, *node + buffer_size());   
			data_allocator::deallocate(*node, buffer_size());
		}
		
		if(start.node != finish.node){
			destroy(start.cur, start.last);
			destroy(finish.first, finish.cur);
			data_allocator::deallocate(finish.first, buffer_size());
		}else{
			destroy(start.cur, finish.cur);
		}
		
		finish = start;
	}
	
	template<class T, class Alloc, size_t BufSize> 
	void Deque<T, Alloc, BufSize>::push_back_aux(const value_type& t){
		value_type t_copy = t;
		reserve_map_at_back(); 
		*(finish.node + 1) = allocator_node();
		construct(finish.cur, t_copy);
		finish.set_node(finish.node + 1);
		finish.cur = finish.first;
	}
	
	template<class T, class Alloc, size_t BufSize>
	void Deque<T, Alloc, BufSize>::push_front_aux(const value_type& t){
		value_type t_copy = t;
		reserve_map_at_front();
		*(start.node - 1) = allocator_node();
		start.set_node(start.node - 1);
		construct(start.cur, t_copy);
		start.cur = start.last - 1;
	}
	
	template<class T, class Alloc, size_t BufSize>
	void Deque<T, Alloc, BufSize>::pop_back_aux(){
		deallocate_node(finish.first);
		finish.set_node(finish.node-1);
		destroy(finish.cur);
		finish.cur = finish.last - 1;
	}
	
	template<class T, class Alloc, size_t BufSize>
	void  Deque<T, Alloc, BufSize>::pop_front_aux(){
		deallocate_node(start.first);
		start.set_node(start.node - 1);
		destroy(start.cur);
		start.cur = start.first;
	}
	
	//// insert_aux()
	
	template<class T, class Alloc, size_t BufSize>
	typename Deque<T, Alloc, BufSize>::iterator Deque<T, Alloc, BufSize>::insert_aux(iterator position, const value_type& x){
		difference_type index = position - start;
		value_type x_copy = x;
		
		if(index < size()/2){
			push_front(front());
			iterator front1 = start;
			++front1;
			iterator front2 = front1;
			++front2;
			position = start + index;
			iterator position1 = position;
			++position1;
			
			Copy(front2, position1, front1);
			
		}else{
			push_back(back());
			
			iterator back1 = finish;
			--back1;
			iterator back2 = back1;
			--back2;
			position = start + index;
			Copy_backward(position, back2, back1);
		}
		*position = x_copy;
		return position;
	}

	template<class T, class Alloc, size_t BufSize>
	void Deque<T, Alloc, BufSize>::insert_aux(iterator position, size_type n, const value_type& x){
		const difference_type elems_before = position - start;
		
		size_type length = size();
		value_type x_copy = x;
		
		if(elems_before < length/2){
			iterator new_start = reserve_elements_at_front(n);
			
			iterator old_start = start;
			position = start + elems_before;
			if(elems_before >= difference_type(n)){
				iterator start_n = start + difference_type(n);
				uninitialized_copy(old_start, start_n, new_start);
				start = new_start;
				Copy(start_n, position, old_start);
				fill(position - difference_type(n), position, x_copy);
			}else{
				__uninitialized_copy_fill(start, position, new_start, start, x_copy);
				start = new_start;
				fill(old_start, position, x_copy);
			}

//			uninitialized_copy(start, position, new_start);
//			start = new_start;
//			fill(start + elems_before, position, x_copy);
		}else{
			const difference_type elems_after = difference_type(length) - elems_before;
			
			iterator new_finish = reserve_elements_at_back(n);
			iterator old_finish = finish;
			position = finish - elems_after;
			if(elems_after >= difference_type(n)){
				iterator finish_n = finish - difference_type(n);
				uninitialized_copy(finish_n, finish, finish);
				finish = new_finish;
				Copy_backward(position, finish_n, old_finish);
				fill(position, position + difference_type(n), x_copy);
			}else{
				__uninitialized_fill_copy(finish, position + difference_type(n), x_copy, position, finish);
				finish = new_finish;
				fill(position, old_finish, x_copy);
			}
			
//			Copy(position, finish, new_finish - elems_after);
//			finish = new_finish;
//			fill(position, position + n, x_copy);
		}
	}

	template<class T, class Alloc, size_t BufSize>
	void  Deque<T, Alloc, BufSize>::insert(iterator position, size_type n, const value_type& x){
		if(position.cur == start.cur){
			iterator new_start = reserve_elements_at_front(n);
			uninitialized_fill(new_start, start, x);
			start = new_start;
		}else if(position.cur == finish.cur){
			iterator new_finish = reserve_elements_at_back(n);
			uninitialized_fill(finish, new_finish, x);
			finish = new_finish;
		}else
			insert_aux(position, n, x);
	}

	template<class T, class Alloc, size_t BufSize>
	void Deque<T, Alloc, BufSize>::new_elements_at_front(size_type new_elements){
		size_type new_nodes = (new_elements + buffer_size() - 1) / buffer_size();
		reserve_map_at_front(new_nodes);
		size_type i;
		for(i=1; i<=new_nodes; ++i)
			*(start.node - i) = allocator_node();
	}
	
	template<class T, class Alloc, size_t BufSize>
	void Deque<T, Alloc, BufSize>::new_elements_at_back(size_type new_elements){
		size_type new_nodes = (new_elements + buffer_size() - 1) / buffer_size();
		reserve_map_at_back(new_nodes);
		size_type i;
		for(i=1;i<=new_nodes;++i)
			*(finish.node + i) = allocator_node();
	}
	
/*                                  test function                            */	

namespace lw00{
	void test00(){
		Deque<int> d;
		
		for(int i=0; i<10; ++i){
			d.push_back(i);
//			d.push_front(-i);
		} 
		
		cout << "no pop_back() d.size()  " << d.size() << " d[0]:  " << d[0] << endl; 
 		
		for(int i=0; i<d.size(); ++i){
			cout << d[i] << endl;
		}
		 
		d.pop_back();
		d.pop_front();
		
		cout << "pop_back()    d.size()  " << d.size() << " d[0]:  " << d[0] << endl;
 		
		for(int i=0; i<d.size(); ++i){
			cout << d[i] << endl;  // d = 1,2,3,4,5,6,7,8
		}
		
		cout << "this is my deque! " << endl;
		cout << "Map size " << d.Map_size() << endl;
		cout << "buffer size " << d.get_buffer_size() << endl;
	}
	
	// copy 有写关于iterator的函数 
	void test01(){
		Deque<int> d;
		
		for(int i=0; i<10; ++i){
			d.push_back(i);
		}
		
		cout << "no erase(it + 3)    d.size()  " << d.size()  << endl;
 		
		for(int i=0; i<d.size(); ++i){
			cout << d[i] << endl;  // d = 0,1,2,3,4,5,6,7,9
		}
		
		Deque<int>::iterator it = d.begin();
		d.erase(it + 3);
		
		cout << " erase(it + 3)     d.size()  " << d.size()  << endl;
 		
		for(int i=0; i<d.size(); ++i){
			cout << d[i] << endl;  // d= 0, 1, 2, 4, 5, 6, 7, 8, 9
		}
		
		cout << " front() " << d.front() << endl;
		cout << " back() " << d.back() << endl;
	}
	
	void test02(){
		Deque<int> d;
		
		for(int i=0; i<10; ++i){	d.push_back(i);	}
		
		cout << "no insert(it + 3)    d.size()  " << d.size()  << endl;
		
		for(int i = 0; i<d.size(); ++i){
			cout << d[i] << "  " << endl; // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 
		}
		
		Deque<int>::iterator it = d.begin();
		d.insert(it+3, 1000);
		cout << "-----------------------------------" << endl << endl;
		cout << "no insert(it + 3)    d.size()  " << d.size()  << endl;
		
		for(int i = 0; i<d.size(); ++i){
			cout << d[i] << "  " << endl; // 0, 1, 2, 1000, 3, 4, 5, 6, 7, 8, 9
		}	
	}
	
	// 犯错原因 ：Copy 函数中没有加上 --n  
	void test03(){
		Deque<int> d;
		
		for(int i=0; i<10; ++i){	d.push_back(i);	}
		
		cout << "no insert(it + 3)    d.size()  " << d.size()  << endl;
		
		for(int i = 0; i<d.size(); ++i){
			cout << d[i] << "  " << endl; // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 
		}
		
		Deque<int>::iterator it = d.begin();
		d.insert(it+3);
		cout << "-----------------------------------" << endl << endl;
		cout << "no insert(it + 3)    d.size()  " << d.size()  << endl;
		
		for(int i = 0; i<d.size(); ++i){
			cout << d[i] << "  " << endl; // 0, 1, 2, 1000, 3, 4, 5, 6, 7, 8, 9
		}	
	}

	void test04(){
		Deque<int> d;
		
		for(int i=0; i<10; ++i){	d.push_back(i);	}
		
		cout << "no insert(it + 3, 10, 1000)    d.size()  " << d.size()  << endl;
		
		
		for(int i = 0; i<d.size(); ++i){
			cout << d[i] << "  " << endl; // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
		}
		
		cout << endl;
		
		Deque<int>::iterator it = d.begin();
		d.insert(it + 6, size_t(10), 1000);
		
		cout << "-----------------------------------" << endl << endl;
		cout << "no insert(it + 3, 10, 1000)    d.size()  " << d.size()  << endl;
		
		for(int i = 0; i<d.size(); ++i){
			cout << d[i] << "  " << endl; // 0, 1, 2, 1000*10, 3, 4, 5, 6, 7, 8, 9
		}
	}
	
	void test05(){
		string s1("abc");
		string s2("abd");
		string s3("abe");
		string s4("abf");
		string s5("abg2");
		
		Deque<string> d;
		d.push_back(s1);
		d.push_back(s2);
		d.push_back(s3);
		d.push_back(s4);
		d.push_back(s5);
		
		for(int i = 0; i<d.size(); ++i){
			cout << d[i] << "  " << endl; // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
		}
	}
}

namespace lw01{
	
	class Stone{
	friend ostream& operator<<(ostream& out, Stone& stone);
	private:
		int m_weight;
		char* m_color; // 4+1 = 5 -> 8
	
	public:
		Stone():m_weight(0), m_color("black"){	}
		
		Stone(const int& weight, const char* color){
			this->m_weight = weight;
			m_color = new char[strlen(color) + 1];
			strcpy(m_color, color);
		}
		
		Stone(const Stone& stone){
			this->m_weight = stone.m_weight;
			m_color = new char[strlen(stone.m_color) + 1];
			strcpy(m_color, stone.m_color);
		}
	};
	
	ostream& operator<<(ostream& out, Stone& stone){
	out << "weight " << stone.m_weight << " color " << stone.m_color;
	return out;
	}
	//---------------------------                                     test functions
	void test00(){
		Deque<Stone> d;
		
		Stone s1(10, "abc");
		Stone s2(11, "bcd");
		Stone s3(12, "cde");
		
	//	cout<< "sizeof(Stone)  " << sizeof(s1) << endl; //8
	//	cout<< "sizeof(Stone)  " << sizeof(s2) << endl; //8
	//	cout<< "sizeof(Stone)  " << sizeof(s3) << endl; //8
		
		d.push_back(s1);
		d.push_back(s2);
		d.push_back(s3);
		
		for(Deque<Stone>::iterator it=d.begin(); it!=d.end(); ++it){
			cout << *it << endl;
			}
	}
	
	void test01(){
		Deque<Stone> d;
		
		Stone s1(10, "abc");
		Stone s2(11, "bcd");
		Stone s3(12, "cde");
		
	//	cout<< "sizeof(Stone)  " << sizeof(s1) << endl; //8
	//	cout<< "sizeof(Stone)  " << sizeof(s2) << endl; //8
	//	cout<< "sizeof(Stone)  " << sizeof(s3) << endl; //8
		
		d.push_back(s1);
		d.push_back(s2);
		d.push_back(s3);
		d.push_back(s1);
		d.push_back(s2);
		d.push_back(s3);
		d.push_back(s1);
		d.push_back(s2);
		d.push_back(s3);
		
		cout << "-----------------------------------" << endl << endl;
		cout << "                         d.size()  " << d.size()  << endl;

		for(Deque<Stone>::iterator it=d.begin(); it!=d.end(); ++it){
			cout << *it << endl;
		}
		
		cout << endl;
		
		Deque<Stone>::iterator it = d.begin();
		d.insert(it + 6, size_t(10), Stone());
		
		cout << "-----------------------------------" << endl << endl;
		cout << "insert(it + 3, 10, 1000)    d.size()  " << d.size()  << endl;
		
		for(Deque<Stone>::iterator it=d.begin(); it!=d.end(); ++it){
			cout << *it << endl;
		}
	}
}

int main(){
//	lw01::test01();
//	lw00::test00();
//	lw00::test01();
//	lw00::test02();
//	lw00::test03();
//	lw00::test04(); 
//	lw01::test01();
	lw00::test05();
	return 0;
}
