add_custom_target(libc_linux_x86_64_tests)
add_dependencies(libc_linux_tests libc_linux_x86_64_tests)

add_libc_unittest(
  libc_linux_x86_64_syscall_unittest
  SUITE libc_linux_x86_64_tests
  SRCS syscall_test.cpp
  DEPENDS
   syscall_impl_h
   support_common_h
)
