## Usage

Proline is a command-line sampling profiler that analyzes the performance of a target executable.

### Command Syntax

```bash
proline [options] <executable> [executable-args]
```

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `--freq <hz>` | Sampling frequency in Hz | `1000` |
| `--max-frames <n>` | Maximum stack depth per sample | `64` |
| `--output <file>` | Write the profiling report to a file | — |
| `--format <type>` | Report format: `text`, `json`, or `folded` | `text` |
| `--verbose` | Enable detailed diagnostics | Disabled |
| `--debug` | Display profiler runtime information | Disabled |
| `--help` | Display help information | — |
| `--version` | Display the profiler version | — |

### Examples

**Basic profiling**

Profile an executable using the default settings.

```bash
proline ./program.exe
```

**Custom sampling frequency and stack depth**

Profile an executable at 2000 Hz with a maximum stack depth of 128 frames.

```bash
proline --freq 2000 --max-frames 128 ./program.exe
```

**Export profiling results**

Save the profiling report in JSON format.

```bash
proline --format json --output report.json ./program.exe
```

**Profile an executable with arguments**

Pass command-line arguments directly to the target executable.

```bash
proline --freq 1000 ./program.exe --input data.txt
```

**Enable detailed diagnostics**

```bash
proline --verbose --debug ./program.exe
```
