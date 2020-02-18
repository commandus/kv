# loracli


Клиент LoraWAN


## Схема передачи данных с датчиков

```
    +--------------+
    |    Vega API  | РЦИТ
    +--------------+
           | 
--------------------------------------------------- ws://vega.rcitsakha.ru:8002
           | src
           | rawdata
    +--------------+                 +--------------+
    | lora-cli     | <---------------| config file  |
    +--------------+                 +--------------+
           |                                                      
           | 1. list devices                                  +-----------------------+  
           | 2. last loaded packet# from each device   <------|  eui->lastRecvPacket# |   +-------------------+
           | 3. load new packets                              |  lora-pq, dbm         |-->| key/value storage |
           | 4. remember last loaded packet# ---------------->|                       |   +-------------------+
           |                                                  +-----------------------+
           +----------------------------------
           | 
           | insert into db
           |
    +---------------+
    | psql rawtable |  
    +---------------+
           |
--------------------------------------------------- lora-mgr
           |
    +---------------+
    | notify listen | on insert pg_notify
    +---------------+
           |
    +----------------+
    | type# request  |---------------------------+
    +----------------+      src, rawdata ->      |
           |                                     | match proto file by src, rawdata
    +----------------+      <-type#              |
    | add type#      |----------------------+    |
    +----------------+                      |    |
           | src                            |    | read type# from proto files
           | rawdata                        |    |
           | type#                          |    |
    +--------------+                     +--------------+
    | gRPC service |                     |  proto files | file system
    +--------------+                     +--------------+
           | type#                              |
           | src                                |
           | rawdata                            |
--------------------------------------------------- lora-mgr	   
--------------------------------------------------- iridium.ysn.ru
--------------------------------------------------- ??-cli
           |                                    |
    +--------------+      type#  ->  +------------------+
    | gRPC client  |-----------------| Proto file cache |<-- sync cache
    +--------------+                 +------------------+
           |                                    |
    +--------------+                            |
    | pretty print |----------------------------+
    +--------------+
           |
           o
           -
           ^
```
## Usage

### Set credentials

```
lora-cli save -u USER -p PASSWORD -h ws://vega.rcitsakha.ru:8002 -v
/home/andrei/.lora-cli saved successfully.
```

### Check connection

```
lora-cli ping
ws://vega.rcitsakha.ru:8002
```

### Get device list

Output eui
```
lora-cli eui
7665676173693133
7665676173683032
```

## Build

Install dependencies first.

### Linux

```
./autogen.sh 
./configure
make
sudo make install
```

### Windows

```
mkdir build
cd build
cmake ..
make
cpack
```

## Third party dependencies

In reason libuwsc depends on libev, and libuwsc/libev does not support in the Windows OS
(no IOCP in libuwsc implemented), zaphoyd/websocketpp.hpp is used in cmake to build Visual
Studio solution.

### C++98

For Centos 6(nova.ysn.ru) compatibility

- OpenSSL
- argtable3

- rapidjson
- (libuwsc)[https://github.com/zhaojh329/libuwsc]
- libev

### C++11

For Windows compatibility

- OpenSSL
- argtable3

- nlohmann/json.hpp	
- websocketpp/websocketpp.hpp

### libev

Linux:

```
git clone https://github.com/enki/libev.git
cd libev/
chmod a+x ./autogen.sh
./autogen.sh
./configure
make
sudo make install
```

Windows:

```
git clone https://github.com/disenone/libev-win
```

### libuwsc

```
git clone --recursive https://github.com/zhaojh329/libuwsc.git
cd libuwsc
mkdir build && cd build
cmake ..
make && sudo make install
sudo ldconfig
```
### Windows

CMakeLists.txt:
```
+set(LIBEV_INCLUDE_DIR "D:/s/loracli/libs/include")
+set(LIBEV_LIBRARY "D:/s/loracli/libs/32/libev_static.lib")
```

src/CMakeLists.txt:
```
-add_definitions(-O -Wall -Werror --std=gnu99 -D_GNU_SOURCE)
+add_definitions(-Wall --std=gnu99 -D_GNU_SOURCE)
```

#### Add C++ compatibility

- uwsc/buffer.h add type conversion
- uwsc/uwsc.h add extern C declaration

```
sudo chmod a+w /usr/local/include/uwsc/buffer.h

-uint8_t *p = buffer_put(b, 1);
+uint8_t *p = (uint8_t*) buffer_put(b, 1);
```

```
sudo chmod a+w /usr/local/include/uwsc/uwsc.h
vi /usr/local/include/uwsc/uwsc.h
#ifndef _UWSC_H
#define _UWSC_H
#ifdef __cplusplus
extern "C" {
#endif
..
#ifdef __cplusplus
}
#endif
#endif
```

### OpenSSL
```
wget https://www.openssl.org/source/openssl-1.1.1c.tar.gz
tar -xf openssl-1.1.1c.tar.gz
cd openssl-1.1.1c
./config
make
sudo make install
```
