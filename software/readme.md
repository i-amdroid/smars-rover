Software
========

Sketches are built with [PlatformIO](https://platformio.org/).

See https://docs.platformio.org/en/latest/core/quickstart.html

Commands
--------

### Web UI

Open PlatformIO Home in your browser.

```sh
pio home
```

### Set default folder

```sh
pio settings set projects_dir /Users/iamdroid/dev/arduino
```

### List boards

```bash
pio boards espressif32
pio boards atmelavr
```

Project commands
----------------

Executing From folder with `platformio.ini`.

### Run project (build)

```sh
pio run
```

### Upload

```sh
pio run -t upload
```

### Upload to specific device only (when multiple devices are configured)

```sh
pio run -e nanoatmega328 -t upload
```

### Upload and run monitor

```sh
pio run -t upload -t monitor
```

### Static Code Analysis

From folder with `platformio.ini`.

```sh
pio check
```

### Serial Monitor

```sh
pio device monitor -b 115200
```

### Reload project

Use this action to update your project following the changes in `platformio.ini`.

```sh
pio project init
```

### Update platforms

Update Platforms, Toolchains, etc.

```sh
pio pkg update
```
