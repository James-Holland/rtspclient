## Suspected bug in rtspsrc plugin in gstreamer 1.8.3 (good-plugins)

There seems to be a bug in rtspsrc. There is a callback for the select-stream signal, through which one can select which stream they wish to SETUP/PLAY. (By returning true in the callback).

When TCP transport is set, via the protocols flag, it breaks if the select-stream callback function is called (and a stream of index greater than 1 is selected).

If TCP transport is set and the select-stream callback isn't used, the stream is played completely fine. This can be seen in wireshark.

The example rtsp-server provided by gstreamer is used as a control server for this demo. The number of streams was increased, as to show the stream selection error. You must use this forked server repo [https://github.com/James-Holland/gst-rtsp-server](https://github.com/James-Holland/gst-rtsp-server)


#### Compiling
##### Test client
```sh  
$ git clone https://github.com/James-Holland/rtspclient.git
$ cd rtspclient
$ mkdir build && cd build
$ cmake .. && make
```
##### Test server
```sh  
$ git clone https://github.com/James-Holland/gst-rtsp-server.git
$ cd gst-rtsp-server
$ git checkout 1.8.3 
$ ./autogen.sh && make
$ cd examples
$ ./test-readme
```
##### Running
From inside the build folder:

###### Working configurations:
Running in UDP mode with select stream callback enabled:
```sh  
$ ./testrtsp -c -t udp
```

Running in UDP mode without select stream callback enabled:
```sh  
$ ./testrtsp -t udp
```

Running in TCP mode without select stream callback enabled:
```sh  
$ ./testrtsp -t tcp
```

###### Configuration that doesn't work:
Running in UDP mode with select stream callback enabled:
```sh  
$ ./testrtsp -c -t tcp
```





