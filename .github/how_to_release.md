# How to release gdal-grass

1. Release tag must be created on CLI (letting GitHub create the tag in
   connection with with GH online publish will **not** create and archive
   the release files):

   ```bash
   git switch main
   git pull
   git status

   # Set version, do not forget!
   version="x.y.z"

   # Assuming 'upstream' points to the OSGeo fork
   commit=$(git rev-parse HEAD)
   tag_message="gdal-grass ${version}"
   git tag -a -m "${tag_message}" $version $commit
   git push --tags upstream
   ```

2. Create a new release on GH, based on the above created tag. "Publish release"
   will activate the *publish.yml* workflow, which will create and add the
   packages to the release.

3. Transfer the packaged files to <https://download.osgeo.org/gdal-grass/>.
