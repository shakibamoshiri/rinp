<br>
<p id="top" align="center">
  <a href="#usage">
    <img src="./res/usage.svg" />
  </a>
  <a href="#screenshot">
    <img src="./res/screenshot.svg" />
  </a>
  <a href="#examples">
    <img src="./res/examples.svg" />
  </a>
  <a href="#goal">
    <img src="./res/goal.svg" />
  </a>
  <a href="#bottom">
    <img src="./res/bottom.svg" />
  </a>
  <a href="https://github.com/k-five/rinp/blob/master/LICENSE">
    <img src="./res/license.svg" />
  </a>
</p>

<br>
<br>

<a href="https://github.com/k-five/rinp">
  <img src="https://github.com/k-five/rinp/blob/master/res/rinp.banner.gif" />
</a>

<br>
<br>

<p align="center">
welcome to a Simple, Beautiful, Modern CLI application<br>
for running multi-commands in parallel.
</p>

<br>
<br>

<h1 id="usage" align="center">usage</h1>
<p align="center">
  &#9899; usage
  &#9898; <a href="#screenshot">screenshot</a>
  &#9898; <a href="#examples">examples</a>
  &#9898; <a href="#goal">goal</a>
  &#9898; <a href="#bottom">bottom</a>
  &#9898; <a href="#top">top</a>
</p>
<br>


 1. `git clone https://github.com/k-five/rinp`
 2. `cd rinp/src/`
 3. `make build`

sample output after `make build`

```
build mode:
compile: main.c rinp.c... okay
linking: main.o rinp.o... okay
Now you can install it by: sudo cp rinp /usr/bin/
"sude" is for root privilege. Use your own.
```

<br>
<br>

<h1 id="screenshot" align="center">screenshot</h1>
<p align="center">
  &#9898; <a href="#usage">usage</a>
  &#9899; screenshot
  &#9898; <a href="#examples">examples</a>
  &#9898; <a href="#goal">goal</a>
  &#9898; <a href="#bottom">bottom</a>
  &#9898; <a href="#top">top</a>
</p>
<br>

Here is a screen-shot of running **10** `sleep` commands simultaneously. One of them is failed and others are succeeded.

<br>
<a href="https://github.com/k-five/rinp">
  <img src="https://github.com/k-five/rinp/blob/master/res/screenshot.png" />
</a>

<br>
<br>

<h1 id="examples" align="center">examples</h1>
<p align="center">
  &#9898; <a href="#usage">usage</a>
  &#9898; <a href="#screenshot">screenshot</a>
  &#9899; examples
  &#9898; <a href="#goal">goal</a>
  &#9898; <a href="#bottom">bottom</a>
  &#9898; <a href="#top">top</a>
</p>
<br>

As a quick start, you can use `echo -e` command. `-e` is for handing special characters like newline (= `'\n'`) and so on.  

  - `echo -e '1\ntwo\n3' | rinp -le sleep` runs **3** `sleep` commands which 1 of them is failed and the other two are succeed.
  - `echo -e '1\n\n\n\n  | rinp -le sleep` runs only one `sleep` command, because newlines are ignored when they are just empty lines.

If you liked to see behind the scene then try using `watch` command as follows:  
`watch -n 1 -exec ps --forest -g -p process-id-of-your-terminal`  
for watching all the children of your Terminal every 1 second. You will face something like this:  

<a href="https://www.gnu.org/licenses/gpl-3.0.en.html">
  <img src="https://github.com/k-five/rinp/blob/master/res/screenshot.watch.png" />
</a>

<br>
<br>

<h1 id="goal" align="center">goal</h1>
<p align="center">
  &#9898; <a href="#usage">usage</a>
  &#9898; <a href="#screenshot">screenshot</a>
  &#9898; <a href="#examples">examples</a>
  &#9899; goal
  &#9898; <a href="#bottom">bottom</a>
  &#9898; <a href="#top">top</a>
</p>
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
<p align="center">
  &#9898; <a href="#usage">usage</a>
  &#9898; <a href="#screenshot">screenshot</a>
  &#9898; <a href="#examples">examples</a>
  &#9898; <a href="#gaol">goal</a>
  &#9899; bottom
  &#9898; <a href="#top">top</a>
</p>
<p id="bottom" align="center">
  rinp copyright &copy; 2017 Shakiba
  <br>
  <br>
  ▒█▀▀█ ▒█▀▀█ ▒█░░░ █▀▀█<br>
  ▒█░▄▄ ▒█▄▄█ ▒█░░░ ░░▀▄<br>
  ▒█▄▄█ ▒█░░░ ▒█▄▄█ █▄▄█<br>
</p>
