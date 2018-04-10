<h1 id="top" align="center">
  <a href="#screenshot">screenshot</a>
  | <a href="#example">example</a>
  | <a href="#goal">goal</a>
  | <a href="#license">license</a>
</h1>
<br>
<br>

<a href="https://github.com/k-five/rinp">
  <img src="https://github.com/k-five/rinp/blob/master/rec/rinp.banner.gif" />
</a>

<br>
<br>

<p align="center">
welcome to A Simple Beautiful Modern CLI application<br>
for running multi-commands in parallel.
</p>


<br>
<br>
<h1 id="screenshot" align="center">screenshot</h1>
<br>

Here is a screen-shot of running **10** `sleep` commands simultaneously. One of them is failed and others are succeeded.
<br>
<br>
<a href="https://github.com/k-five/rinp">
  <img src="https://github.com/k-five/rinp/blob/master/rec/screenshot.png" />
</a>

<br>
<br>
<h1 id="example" align="center">example</h1>
<br>

As a quick start, you can use `echo -e` command. `-e` is for handing special characters like newline (= `'\n'`) and so on.  

  - `echo -e '1\ntwo\n3' | rinp -le sleep` runs **3** `sleep` commands which 1 of them is failed and the other two are succeed.
  - `echo -e '1\n\n\n\n  | rinp -le sleep` runs only one `sleep` command, because newlines are ignored when they are just empty lines.

If you liked to see behind the scene then try using `watch` command as follows:  
`watch -n 1 -exec ps --forest -g -p process-id-of-your-terminal`  
for watching all the children of your Terminal every 1 second. You will face something like this:  

<a href="https://www.gnu.org/licenses/gpl-3.0.en.html">
  <img src="https://github.com/k-five/rinp/blob/master/rec/screenshot.watch.png" />
</a>

<br>
<br>
<h1 id="goal" align="center">goal</h1>
<br>


This app is suitable for running muilt-lengthy commands that you do NOT want to see their outputs. I had a lot of `.mp4` files
that should be converted to `.mp3`. Of course I could use `xargs` with redirection (= `&> /dev/null`), but I also liked to see
if the commands are failed or succeeded.<br>
Plus practicing some advanced techniques in `C` and `Linux Programming` that I have learned and some tiny of UI/UX.
I daily use it for:

`ls *.mp4 | rinp -le ffmpeg -i {} {}.mp3`

which converts `mp4` files to `.mp3`s. Of course you can use it for `cp` or `mv` or anything you like.
I also tried to add some comments to the code, so you can read it as an educational code.

<br>
<br>
<h1 id="license" align="center">license</h1>
<br>
<p align="center">
<a href="https://www.gnu.org/licenses/gpl-3.0.en.html">
  License GPL-3
</a>
</p>
