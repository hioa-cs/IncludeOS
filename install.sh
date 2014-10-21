#! /bin/bash

# Installation script for IncludeOS toolchain + IncludeOS libraries

OSDIR=/usr/local/IncludeOS/

echo -e "\n>>> Looking for toolchain"
if [ ! -f $OSDIR/bin/i686-elf-c++ ] 
then
    echo "  * Toolchain missing - installing..."
    ./etc/install_toolchain.sh
else
    echo "  * Toolchain seems OK "
fi

echo -e "\n>>> Installing vmbuilder"
cd vmbuild
make
sudo cp vmbuild $OSDIR/

echo -e "\n>>> Installing IncludeOS"
cd ../src
make 
sudo make install

echo -e "\n >>> Done. Test the installation by running ./test.sh \n"

