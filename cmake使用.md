# cmake学习记录

## 库链接
target_link_libraries与link_libraries都可用于库链接，区别在于影响范围以及使用位置。target_link_libraries要在add_executable之后，link_libraries在之前。

链接库的形式有很多
1. add_library形成的目标。可以是编译形成的，也可以是：INTERFACE纯头文件库。IMPORTED将已有的目标文件构建为目标，用set_target_properties设置库路径，对于windows上的dll，还要设置lib即导入库的路径，与linux.so不同，dll的符号表默认不公开，需要使用__declspec(dllexport)或lib文件导出符号。
2. 直接用库路径
3. 可以用 Find库名.cmake(Module模式) 或 库名Config.cmake(Config模式)文件封装各种预定义变量，利用find_package应用。Module模式从CMAKE_MODULE_PATH中寻找文件，Config模式从 库名_DIR 路径中寻找