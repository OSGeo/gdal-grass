# GDAL-GRASS driver test suite

## How to run tests

1. You need to install `pytest` to run the test suite. This should do it:

   ```bash
   pip install --upgrade pip
   pip install pytest pytest-env pytest-sugar gdal==$(gdal-config --version)
   ```

2. Configure

   ```bash
   cd gdal-grass
   cmake -B build
   cd build
   cmake build .
   ```

3. Then, run tests with:

   ```bash
   ctest
   ```

   or:

   ```bash
   cd autotest # i.e gdal-grass/build/autotest
   pytest
   ```

4. Some quick usage tips:

   ```bash
   # get more verbose output; don't capture stdout/stdin
   pytest -vvs

   # run all the ogr tests
   pytest ogr

   # run a particular module only
   pytest ogr/ogr_grass.py

   # run a particular test case in a module
   pytest ogr/ogr_grass.py::test_ogr_grass_point2
   ```
