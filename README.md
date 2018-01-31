<a href="https://github.com/k-five/rinp">  
  <img src="https://github.com/k-five/rinp/blob/master/rec/rinp.banner.gif" />  
</a>  


<hr>



█░░░█ █▀▀ █░░ █▀▀ █▀▀█ █▀▄▀█ █▀▀  
█▄█▄█ █▀▀ █░░ █░░ █░░█ █░▀░█ █▀▀  
░▀░▀░ ▀▀▀ ▀▀▀ ▀▀▀ ▀▀▀▀ ▀░░░▀ ▀▀▀  
to A Simple Beautiful Modern CLI application  
for running multi-commands in parallel.  


<a href="https://github.com/k-five/rinp">  
  <img src="https://github.com/k-five/rinp/blob/master/rec/screenshot.svg" />  
</a>  

Here is a screen-shot of running **10** `sleep` commands at the same time. One of them is failed and others are succeed.  
<a href="https://github.com/k-five/rinp">  
  <img src="https://github.com/k-five/rinp/blob/master/rec/screenshot.png" />  
</a>  


<br>  
<a href="https://github.com/k-five/rinp">  
  <img src="https://github.com/k-five/rinp/blob/master/rec/examples.svg" />  
</a>  

Aa a quick start, you can use `echo -e` command. `-e` is for handing special characters like newline (= `'\n'`) and so on.  

  - `echo -e '1\ntwo\n3' | rinp -le sleep` runs **3** `sleep` commands which 1 of them is failed and the other two are succeed.  
  - `echo -e '1\n\n\n\n  | rinp -le sleep` runs only one `sleep` command, because newlines are ignored when they are just empty lines.  

If you liked to see behind the scene then try using `watch` command as follows:  
`watch -n 1 -exec ps --forest -g -p process-id-of-your-terminal`  
for watching all the children of your Terminal every 1 second. You will face something like this:  
  
<a href="https://www.gnu.org/licenses/gpl-3.0.en.html">  
  <img src="https://github.com/k-five/rinp/blob/master/rec/screenshot.watch.png" />  
</a>  



<br>  
<a href="https://www.gnu.org/licenses/gpl-3.0.en.html">  
  <img src="https://github.com/k-five/rinp/blob/master/rec/gole.svg" />  
</a>  

This app is suitable for running muilt-lengthy commands that you do NOT want to see their outputs. I had a lot of `.mp4` files  
that should be converted to `.mp3`. Of course I could use `xargs` with redirection (= `&> /dev/null`), but I also liked to see  
if the commands are failed or succeed.  
Plus practicing some advanced techniques in `C` and `Linux Programming` that I have learned.  
I daily use it for:  
  
`ls *.mp4 | rinp -le ffmpeg -i {} {}.mp3`
  
which converts `mp4` files to `.mp3`s. Of course you can use it for `cp` or `mv` or anything you like.  
I also tried to add some comments to the code, so you can read it as an educational code.  

<br>  
<a href="https://www.gnu.org/licenses/gpl-3.0.en.html">  
  <img src="https://github.com/k-five/rinp/blob/master/rec/license.svg" />  
</a>  
  

<br>  
<a href="https://www.gnu.org/licenses/gpl-3.0.en.html">  
  License GPL-3  
</a>  
