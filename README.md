# Coroutine
带有共享栈缓存的简易协程库
- 通过Comanager创建协程调度器，一个调度器可以有多个协程
- 支持多级协程，使用co_record记录协程调用层次
- 通过coresume唤醒协程，coyield返回上一级协程
- 实现了共享栈缓存，调度器内部有多个共享栈，根据协程id哈希选择对应的共享栈
- 使用C++11的std::functional为协程绑定函数，更加方便

运行结果：
  main
  func1
  main
  func2
  main
  func4
  main
  func2
  main
  func1
  main
  func4
  main
  func1
  main
  func4
  main
  func2
  func3
  func2
  func5
  main2
  func3
  main2
  func5
  0000
  1
