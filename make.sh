BUILD_ARCH=armv7a

CUR_DIR=`eval pwd`
JNI_MK=android/ijkplayer/ijkplayer-$BUILD_ARCH/src/main/jni/Android.mk
git checkout $JNI_MK
echo "include $CUR_DIR/ijkmedia/*.mk" >> $JNI_MK
echo "include $CUR_DIR/ijkprof/android-ndk-profiler-dummy/jni/*.mk" >> $JNI_MK

cd config
rm module.sh
ln -s module-default.sh module.sh

cd ..
./init-android.sh

cd android/contrib
./compile-ffmpeg.sh clean
./compile-ffmpeg.sh $BUILD_ARCH

cd ..
./compile-ijk.sh clean
./compile-ijk.sh $BUILD_ARCH

cd ..
mkdir dist
cd dist
rm -rf *
cp -r ../android/ijkplayer/ijkplayer-$BUILD_ARCH/src/main/libs/* ./
cd ..
echo Build Successful
ls -lha dist/*/*
