# wordexp.js
wordexp.h brought to node.js.

Expand words like POSIX shell

# note
This module is in development, expansion isn't actually supported yet. But it does find great use in correctly turning an argument string into a correct `argv` array.

# usage

```js
const wordexp = require( '/path/to/wordexp.js' )

//...
var argv = wordexp( input ); 
//...
```

Example:

`single words 'multiple words' "\"escaped\" characters -e --option="value string"`

Will be expanded to:

\[ `single`, `words`, `multiple words`, `"escaped" characters`, `-e`, `--option=value string` ]

```js
> wordexp("single words 'multiple words' \"\\\"escaped\\\" characters\" -e --option=\"value string\"")
[
  'single',
  'words',
  'multiple words',
  '"escaped" characters',
  '-e',
  '--option=value string'
]
```
