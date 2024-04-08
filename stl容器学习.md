# stl容器学习总结

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
