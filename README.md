# BigFS

A File System to store large files on multiple systems. Data blocks are downloaded and uploaded to different FileDataServers in parallel. Commands such as `cat`, `cp`, `mv`, `ls` and `rm` are supported.

## Getting Started

### Compilation

Use `make` to compile the files. It will generate 3 files,

- `fns` (Executable for FileNameServer)
- `fds` (Executable for FileDataServer)
- `bigfs` (Executable for Client)
 
### Running the FileNameServer

```bash
$ ./fns <port number>
```

For example, to run the server at port `5555`,
```bash
$ ./fns 5555
```

### Running the FileDataServer

```bash
$ ./fds <port number>
```

For example, to run the server at port `8081`,
```bash
$ ./fds 8081
```
### Running the Client

```bash
$ ./bigfs <path to config file>
```

For example, if the config file is `config.txt` in the same directory,
```bash
$ ./bigfs config.txt
```

## Format for the config file

The config file should have the following format,

```Java
<FNS IP> <FNS PORT>
<k = Number of FDS>
<FDS0 IP> <FDS0 PORT>
<FDS1 IP> <FDS1 PORT>
..
..
..
<FDS(k-1) IP> <FDS(k-1) PORT>
```

For example,
```
127.0.0.1 5555
3
127.0.0.1 8081
127.0.0.1 8082
127.0.0.1 8084
```

## Commands

`bigfs` is used to represent the remote file system. So, Copying into `bigfs` is equivalent to uploading the the BigFS and copying from `bigfs` is equivalent to downloading from the BigFS.

### cp - Usage Examples


To copy the file `movie.mp4` to a remote directory `movies` with name `movie1.mp4` ,
```console
$ cp movie.mp4 bigfs/movies/movie1.mp4
```

To copy the file `movie1.mp4` from remote directory `movies` to local directory `localmovies` with name `movie1.mp4`
```console
$ cp bigfs/movies/movie1.mp4 localmovies/movie1.mp4
```

To copy all contents of the local directory `localmovies` recursively to root directory of BigFS,
```console
$ cp -r localmovies/ bigfs/
```

To copy the remote directory `movies` and all its contents recursively to local directory `downloads`,
```console
$ cp -r bigfs/movies downloads/
```
**Note**: 

- _`cp` does not support copying files from one remote folder to another remote folder._
- _copying files within local filesystem is supported._

### mv - Usage Examples

To move a file `lecture10.pdf` from the remote directory `study` to remote directory `acads` with name `lecture10.pdf`
```console
$ mv bigfs/study/lecture10.pdf bigfs/acads/lecture10.pdf
```

**Note**:

- _Moving directories is supported._
- _Moving files from local file system to remote file system and vice versa is not supported._
- _Moving files from one local directory to another local directory is supported._

### rm - Usage Examples

To remove a remote file `song1.mp3` from the remote directory `music`,
```console
$ rm bigfs/music/song1.mp3
```

To remove the complete `music` remote directory recursively,
```console
$ rm -r bigfs/music
```
**Note**: _`rm` on local files is supported._

### ls - Usage Examples

To view the files and folders in root remote directory,
```console
$ ls
```

To view the files and folders in a remote directory named `music`,
```console
$ ls bigfs/music
```

**Note**: _`ls`on local directories is supported._

### cat - Usage Examples

To display the file `client.c` in remote directory `code`,
```console
$ cat bigfs/code/client.c
```
**Note**: _`cat`on local files is supported._

## Working

### Temporary Directory

All intermediate files are stored in a temporary directory named `tmp` which is created automatically in the current working directory.

### Spliting a File

- Before spliting a file, an alpha-numeric random string of `10 characters` is generated as a basename for the temporary file. 
- 1MB blocks are created for the file and are stored in the temporary directory.
- Basename of the file is returned by the split function.

For Example, for a file of 3.4MB, suppose the 10 character random string is `pi2ow8xbshsxitc`, then it will have its parts named as `pi2ow8xbshsxitc_0`, `pi2ow8xbshsxitc_1`, `pi2ow8xbshsxitc_2` and `pi2ow8xbshsxitc_3`.
