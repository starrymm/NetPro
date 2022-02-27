# NetWorkProgramming
HttpServer，采用Reactor模式。

1. 主线程接受新的连接请求，并分配给子线程处理I/O；

2. 主线程和子线程都阻塞在epoll_wait；   

3. 当不设置子线程时，主线程既处理连接，又处理I/O。

编译： 
cd NetPro/cpp  

mkdir build && cd build  

cmake ..  

make 

程序运行：

![image](https://user-images.githubusercontent.com/92151722/155872617-bb5ac0f4-a797-4d6e-88da-26acbfc9724b.png)
一个主线程，和8个子线程。


压测机器：
y7000p2021  i7-11800H, 16g

压测结果：

![image](https://user-images.githubusercontent.com/92151722/155872638-2ae8935c-657f-447a-b101-a2703aee14eb.png)
采用压测工具，1w台客户端同时访问服务器5秒，服务器全部接受了请求。

网页截图： 
![image](https://user-images.githubusercontent.com/92151722/155877141-63386dd5-73ec-413d-a158-3758a0121f28.png)


