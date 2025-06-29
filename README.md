# OS-Term-Project: mCertiKOS with Enhanced File Locking

![OS Kernel Demo](https://img.shields.io/badge/OS-Kernel%20Monitor-blue) 
![File Locking](https://img.shields.io/badge/Feature-File%20Locking-green)

A certified kernel extension for mCertiKOS featuring a complete file locking (`flock`) implementation with shared/exclusive locks.


## Features

### mCertiKOS Core
✔ Certified microkernel architecture  
✔ Process and thread management  
✔ Memory protection (MPTIntro/MPTNew)  
✔ Device console I/O  

### Enhanced File Locking (`flock`)
```c
// Example lock operations
flock_acquire(&file_lock, LOCK_EX);  // Exclusive lock
flock_acquire(&file_lock, LOCK_SH);  // Shared lock
flock_release(&file_lock);           // Release lock
```
### Lock Types:

```bash
LOCK_SH: Shared (read) locks

LOCK_EX: Exclusive (write) locks

LOCK_NB: Non-blocking mode
```

### Advanced Features:

```bash
Writer-priority scheduling

Deadlock avoidance

State transition tracking

Integrated condition variables
```

## Build & Run
### Prerequisites
 1. gcc (x86-64 cross-compiler recommended)

 2. make

 3. qemu-system-x86_64 for emulation

### Clone with submodules
```bash
git clone https://github.com/N4M154/OS-Term-Project.git
cd OS-Term-Project
```

### Build the kernel
```bash
make clean
make
```

### Run in QEMU
```bash
make qemu
```


## Usage
### Kernel Monitor Commands
```bash
$> help                  # Show available commands
$> kerninfo              # Display kernel memory layout
$> flock test            # Run automated lock tests
$> flock interactive     # Enter interactive mode
```

### Interactive Flock Tester
```bash
flock> ex        # Acquire exclusive lock
flock> sh        # Acquire shared lock
flock> nb_ex     # Try non-blocking exclusive
flock> print     # Show current lock state
flock> un        # Release active lock
```

### Key Components
- Spinlocks: Atomic kernel locking

- Condition Variables: Blocking/wakeup mechanism

- State Machine: INACTIVE ↔ SHARED ↔ EXCLUSIVE transitions

## Contributors
1. [Nabila Sheona](https://github.com/nabila-sheona)
2. [Namisa Najah](https://github.com/N4M154)
3. [Nazifa Tasneem](https://github.com/nazifatasneem13)
4. [Nusrat Siddique](https://github.com/ns-tuli)
