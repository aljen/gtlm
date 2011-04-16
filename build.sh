 #!/bin/bash
g++ -g -o gc gtlm-console/gtlm-console.cpp libgtlm/libgtlm.cpp -I libgtlm/ -I/usr/include/libusb-1.0  -lusb-1.0 -lconfig
