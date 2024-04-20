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


## Rb_tree
Rb_tree是map、multimap、set、multiset的底层数据结构

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