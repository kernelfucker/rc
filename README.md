# rc
vim-style cli file manager

# compile
$ clang rc.c -o rc

# usage
$ ./rc

# keys
h: move up to parent directory, like **cd ..**

j: move cursor down

k: move cursor up

l: enter directory under cursor but if its a directory

return: open file or directory under cursor

q: quit the rc

# file viewer keys
j: scroll down

k: scroll up

d: scroll down half a page

u: scroll up half a page

f or space: scroll down a full page

b: scroll up a full page

g: jump to top of file

v: jump to bottom file

q: exit file viewer

# command/last mode
:q quit

:q! force quit

:w is normally saved but the placeholder

:wq save and quit

:cd [path] change directory

esc: exit command/last mode

# normal features
vim-style keys

directory listing

text file viewer

basic command/last mode

# example
![image](https://github.com/user-attachments/assets/d037ed36-1fc8-4303-a25f-e10dfec484e8)
