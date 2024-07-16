# Steps Petalinux

## Create Project
`petalinux-create --type project --template zynq --name des3_axi`

### Configure Hardware
`petalinux-config --get-hw-description <PATH-TO-XSA Directory>`

## Create Kernel Module
`petalinux-create -t modules --name des3module --enable`

`petalinux-build -c kernel` to build kernel first, and then
`petalinux-build -c des3module` to build the module

Or simply
`petalinux-build`

## Create Application
`petalinux-create -t apps --name des3app --enable`

`petalinux-build -c des3app` and then
`petalinux-build -c rootfs`

Or simply
`petalinux-build`

## Package Petalinux Image
`petalinux-package --boot --u-boot --fpga <PATH-TO-BITSTREAM> --force`

## Booting PetaLinux Image on Hardware with an SD Card
In the FAT partition
`sudo tar -xzvf ~/Petalinux_projects/des3_proj/images/linux/rootfs.tar.gz .`
In the EXT4 partition
`cp ~/Petalinux_projects/des3_proj/images/linux/BOOT.BIN .`
`cp ~/Petalinux_projects/des3_proj/images/linux/image.ub .`
`cp ~/Petalinux_projects/des3_proj/images/linux/boot.scr .`

# Demo
## Mount the Kernel Module
`sudo modprobe des3module`
`sudo chmod u+rw /dev/des3module`
## Create Files for Demo
### Key File
`echo -n "{key}" > keyfile`
e.g.
`echo -n "6d6e73646e7a6e6261646173313232343437373838313838" > keyfile`
### Plain Text File
`echo {data} > original_plain.txt`

## Execute Application
### TripleDES Encryption
`sudo des3app keyfile original_plain.txt cypher.txt 0`

### TripleDES Decryption
`sudo des3app keyfile cypher.txt decrypted_plain.txt 1`

### Verify Correct Execution
`cat original_plain.txt`
`cat decrypted_plain.txt`

Or better
`diff -s original_plain.txt decrypted_plain.txt`

`cypher.txt` is the output of the encryption process and should be non-human-readable.