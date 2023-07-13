# ArduinoIoTDoorControl

# Doc

- lab/livereport 是设备回报topic

- lab/cmd 命令下发topic

  - opendoor命令

    - a1:[int]
  
    - a2 :[int]
  - isalive? 命令，会在lab/livereport回报i am alive

# Usage

首先修改一下自己的wifi ssid和密码，还有mqtt服务器的相关信息。以及舵机的PWM端口。

程序跑起来之后，会自动连接wifi，然后连接mqtt服务器（运行中如果mqtt断联会自动重连）。

成功连接mqtt服务器之后会每隔30s向lab/livereport频道通报心跳包；收到mqtt消息的时候，会先判断频道是不是lab/cmd，如果是说明是正常的控制信号，然后会解析消息体的内容，做出response。



## TODO

- 把心跳包的逻辑改成询问后回复
