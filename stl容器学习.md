# stl容器学习总结

## allocator
负责内存申请释放，对象构造析构

```c++
//默认allocator,空类
template<typename _Tp>
class new_allocator
{
	//负责对不同类型的对象生成对应allocator
	//eg: allocator<_Val> -> rebind<_Rb_tree_node<_Val> >::other
	template<typename _Tp1>
	struct rebind
	{ typedef new_allocator<_Tp1> other; };

	//内存申请使用 new
	_GLIBCXX_NODISCARD _Tp*
    allocate(size_type __n, const void* = static_cast<const void*>(0))
    {
		return static_cast<_Tp*>(::operator new(__n * sizeof(_Tp)));
	}

	//释放用 delete
	void
    deallocate(_Tp* __p, size_type __t __attribute__ ((__unused__)))
	{
		::operator delete(__p);
	}

	//构造用 placement new
	template<typename _Up, typename... _Args>
	void
	construct(_Up* __p, _Args&&... __args)
	noexcept(std::is_nothrow_constructible<_Up, _Args...>::value)
	{ 
		::new((void *)__p) _Up(std::forward<_Args>(__args)...); 
	}

	//直接调用析构函数
	template<typename _Up>
	void
	destroy(_Up* __p)
	noexcept(std::is_nothrow_destructible<_Up>::value)
	{ __p->~_Up(); }
}
```

## vector

### 内部成员
```c++
template<typename _Tp, typename _Alloc = std::allocator<_Tp> >
    class vector : protected _Vector_base<_Tp, _Alloc>
```
_Vector_base负责内存的申请与释放，对象构造函数与析构函数由vector负责。注释中表示这样使异常安全更容易。

```c++
struct _Vector_impl_data
{
	pointer _M_start;
	pointer _M_finish;
	pointer _M_end_of_storage;
}

//_Vector_base的核心成员变量，该类继承了data和allocator
struct _Vector_impl
	: public _Tp_alloc_type, public _Vector_impl_data;
```

## deque
### 内部成员
```c++
struct _Deque_impl
      : public _Tp_alloc_type, public _Deque_impl_data;

struct _Deque_impl_data
{
	//buffer指针数组
	_Map_pointer _M_map;
	size_t _M_map_size;
	//首尾迭代器
	iterator _M_start;
	iterator _M_finish;
}
```

### 主要方法
```c++
_Deque_impl_data _M_impl;
iterator begin() { return this->_M_impl._M_start; }
iterator end() { return this->_M_impl._M_finish; }
reference operator[](size_type __n)
{
	//利用迭代器
	return this->_M_impl._M_start[difference_type(__n)];
}

//插入
void push_front(const value_type& __x)
{
	if (this->_M_impl._M_start._M_cur != this->_M_impl._M_start._M_first)
	{
		//cur前还有空间，直接用allocator构造
		_Alloc_traits::construct(this->_M_impl,this->_M_impl._M_start._M_cur - 1,__x);
		--this->_M_impl._M_start._M_cur;
	}
	else
		_M_push_front_aux(__x);
}

void _M_push_front_aux(_Args&&... __args)
{
	//检查_M_start._M_node前是否有空间，没有就调整map，两种方式
	//1.如果map的size大于所需buffer数两倍，说明后面还有很多空间，则将数据向后移动
	//2.否则扩容map，new_size=old_size+nodes_to_add+2，并为start finish迭代器设置新的map指针
	_M_reserve_map_at_front();

	//构造新的buffer，放入map中
	*(this->_M_impl._M_start._M_node - 1) = this->_M_allocate_node();

	//移动start迭代器
	this->_M_impl._M_start._M_set_node(this->_M_impl._M_start._M_node - 1);
	this->_M_impl._M_start._M_cur = this->_M_impl._M_start._M_last - 1;

	//构造
	_Alloc_traits::construct(this->_M_impl, this->_M_impl._M_start._M_cur,std::forward<_Args>(__args)...);
}

//删除
void pop_front() _GLIBCXX_NOEXCEPT
{
	//判断析构的同时是否释放buffer
	if (this->_M_impl._M_start._M_cur!= this->_M_impl._M_start._M_last - 1)
	{
		_Alloc_traits::destroy(_M_get_Tp_allocator(),this->_M_impl._M_start._M_cur);
		++this->_M_impl._M_start._M_cur;
	}
	else
		_M_pop_front_aux();
}


```

## _Rb_tree
Rb_tree是map、multimap、set、multiset的底层数据结构

### 内部成员

```c++
struct _Rb_tree_impl
	: public _Node_allocator
	, public _Rb_tree_key_compare<_Key_compare>
	, public _Rb_tree_header
```
_Rb_tree_header是Rb_tree头结点，与树的根节点(如果非空)互为父节点。头结点的左子结点指向树的最小结点，右子结点指向最大结点。
```c++
std::map<int,int> m;
m.insert({1,2});
m.insert({2,3});
auto i=m.begin();
printf("%d %d\n",i->first,i->second); //1,2
++i;++i;
printf("%d\n",i==m.end()); //1
++i;
printf("%d %d\n",i->first,i->second);//2,3
```

### 析构

析构没有用后序遍历，而是遍历树每个左枝，以此为起点递归调用，减少递归深度？
```c++
~_Rb_tree() { _M_erase(_M_begin()); } //实参为根结点

void _M_erase(_Link_type __x)
{
	// Erase without rebalancing.
	while (__x != 0)
	{
		_M_erase(_S_right(__x));
		_Link_type __y = _S_left(__x);
		_M_drop_node(__x);
		__x = __y;
	}
}
```

### set

set不允许修改值，因为键和值类型相同，破坏树结构，且返回的迭代器为以下类型

```c++
typedef typename _Rep_type::const_iterator	 iterator;
typedef typename _Rep_type::const_iterator	 const_iterator;

//访问相关方法均为常函数
template<typename _Tp>
struct _Rb_tree_const_iterator
{
	reference operator*() const;
	_Self& operator++();
}
```

### 使用

_Rb_tree相比_Hashtable空间利用率高，且有顺序，但排序的依据是key。

1. insert相关
   
   set,map的insert有多种重载。与vector，list等无序容器insert需要指定插入位置迭代器不同，红黑树的插入有自己的平衡逻辑，无法指定位置，但有方法影响。
   ```c++
   //常用方式
   std::pair<iterator,bool> insert( value_type&& value );
   //无法指定位置
   //但插入value到尽可能接近，恰好前于hint的位置
   //注意返回值不同
   iterator insert( iterator hint, const value_type& value );

   ```
   
   力扣460LFU，采用红黑树记录排序。使用红黑树要注意排序的比较条件，key的类型是否内置、排序条件是否多个。lfu的排序条件：使用次数+时间戳(次数相同时)。

   方法一：构建包含使用次数和时间戳的结构体_node,重载比较函数，利用set<_node>排序

   方法二: 应为比较条件只有两个，可以使用multimap排序，key为使用次数，内置类型无需重载。考虑到次数相同，插入时使用insert(lower_bound(fre+1),value)，向更多次数之前插入

