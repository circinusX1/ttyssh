# ttyssh
tty-ssh-bridge, A terminal from a serial device to a remote server over ssh

```
cd module
sudo apt install linux-headers-$(uname -r)
cd ..
mkdir build
cd build
cmake ..
make
cp ./ttyssh ..
cd ..
./ttyssh  115200 USER_ON_REMOTE PASSSOWD_ON_REMOTE REMOTE_IP
```

Now you can connect any uart aware terminal to /dev/vpinput0 115200.



![image](https://github.com/user-attachments/assets/1c990d87-003d-4777-9c16-638bcf1b49d0)
