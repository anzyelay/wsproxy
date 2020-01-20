## 项目简介
此项目是由[mongoose](https://www.cesanta.com)修改而来,纯C代码，替代[noVnc](https://novnc.com)项目中的websockets to tcp sockets proxy代理的websockify程序，整合了http服务器和websocket转vnc的代理服务

## 下载编译
此项目使用QtCreator做项目编译，但不依赖另何第三方库（包括Qt), 直接下载编译即可

## 如何使用
- 命令行:  
`dt_wsproxy [options]`  
 options说明
	- **-d** :http服务器根文件目录(此处即是noVnc目录,默认/tmp/noVnc)
	- **-u** :http上传文件目录(默认/tmp)
	- **-p** :http监听端口(默认80)
	- **-h** :显示此帮助选项

 使用示例  
```sh
dt_wsproxy -d /tmp/noVnc -u /path/to/upload -p 80
```




