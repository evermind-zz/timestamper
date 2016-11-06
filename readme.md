# Timestamper aka ts
This tool has just one purpose:

> Prepend a timestamp to data that was piped into it.

The timestamper tool was inspired by moreutils's ts tool.
It is written in C and not in perl like moreutils implementation.

#### Usage:

The default timestamp is `"%b %d %H:%M:%S"`.

- ts supports a time format string as single argument.
- you can adjust the timestamp output as you like:
```bash
echo "some text" | ts "%b %d %H:%M:.%S"
Nov 06 15:42:43.751569 some text
```
