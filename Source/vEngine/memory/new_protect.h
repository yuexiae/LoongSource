//-----------------------------------------------------------------------------
// File: new_protect
// Auth: Lyp
// Date: 2004-2-26	
// Last: 2004-2-26
//-----------------------------------------------------------------------------
#pragma once
namespace vEngine {
//-----------------------------------------------------------------------------
/*	设计观念
 1.	为避免在程序各处频繁进行new返回值检查,这里进行了统一处理
 2.	当内存分配失败的时候,首先尝试从内存池中释放一些未被使用的内存
 3.	提示用户选择是否 退出/重试/继续
*/
//-----------------------------------------------------------------------------
INT NewHandle( size_t size );
}
