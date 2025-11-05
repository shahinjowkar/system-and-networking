# mysh

`mysh` is a mini shell written in C that uses a lexical scanner and Abstract Syntax Tree (AST) to implement shell operations. The shell processes commands through lexical analysis, parsing into an AST, and executing the resulting tree structure.

## Features

- **Command Execution**: Execute external programs and built-in commands (`cd`, `echo`)
- **Piping**: Chain commands using `|` operator
- **Sequential Execution**: Execute commands sequentially using `;`
- **Background Execution**: Run commands in background using `&`
- **Command Blocks**: Group commands using `()` for subshells with shared redirection context
- **Redirection**: Input (`<`), output (`>`), and append (`>>`) redirection
- **Nested Commands**: Support for complex nested structures like `(cmd1 | cmd2) > out.txt`

## Lexical Scanner and Abstract Syntax Tree (AST)

The shell implements a two-phase parsing approach:

1. **Lexical Scanner**: Tokenizes input using `gettoken()` and `peak()` functions, identifying operators (`|`, `;`, `&`, `<`, `>`, `>>`), command names, arguments, and file paths.

2. **AST Parsing**: The tokenized input is parsed into an Abstract Syntax Tree with node types: `EXEC`, `REDIR`, `PIPE`, `SEQ`, and `BG`. Each command is represented as a tree structure where nodes represent different shell constructs (commands, redirections, pipes, sequential execution, background jobs). Execution follows recursive tree traversal, handling pipes, redirections, and built-ins appropriately.

## Grammar

```
parseexec → EXEC REDIR { aaa REDIR }+ ( BLOCK )
parseredirs → REDIR { < | > | >> } aaa
parsepipe → EXEC [ | parsepipe ]
parseline → LINE PIPE { & } [ ; LINE ]
parseblock → BLOCK ( LINE ) REDIR
```

The grammar supports command execution, built-ins, redirection, piping, sequencing (`;`), background execution (`&`), and command grouping (`()`).

## Example Usage

```bash
# Nested commands: pipe within command block with redirection
mySh>> (ls | grep .c) > output.txt
mySh>> (cat file.txt | grep pattern) > results.txt

# Command blocks with redirection applied to entire block
mySh>> (cd /tmp ; ls) > dirlist.txt
mySh>> (echo "test" ; pwd) >> log.txt
```


