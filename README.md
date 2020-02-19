# kv

Simple key/value CLI utility.

## How to use

### Get value

kv KEY

### Set value

kv KEY VALUE

### Remove key/value

kv -d KEY

### Empty value

Set an empty value for the key. The key is not deleted.

kv -0 KEY

## Options

- -p, --dbpath Set the database path
- -f, --dbflags Set the flags of the database file. Default 0
- -m, --dbmode Set the file open mode of the database. Default 0664
- --starts-with If set, search for all records with a key starting with the specified prefix
- -c --config FILENAME Set configuration file name
	
## Build

Install liblmdb-dev
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

- [libmdbx](https://github.com/leo-yuriev/libmdbx) or
- lmdb
- argtable3

### libmdbx

```
git clone https://github.com/leo-yuriev/libmdbx.git
cd libmdbx
make
sudo make install
```
