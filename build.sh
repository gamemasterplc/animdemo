set -e

if [ $1 = "clean" ]; then
  echo "Cleaning up..."
  make clean
  make -C tools/mkanimspr clean
fi

# Tools
echo "Building tools..."
make -C tools/mkanimspr -j4

# Code
echo "Building code..."
make -j4
