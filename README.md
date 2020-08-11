XamarinAndroidNativeDebugTest
=============================

This test case shows that you cannot create an AAB bundle a Release build of an native Android app (containing C++ code compiled to the 4 architecture ABIs).  The Release build AAB bundling process fails because it appears that MSBuild is attempting to bundle `gdbserver` into the AAB.  (Note that the same issue of including `gdbserver` also occurs if bundling as APK, including multiple arch+ABI specific APKs, however in this case including `gdbserver` is not a fatal build error).

Dramatis Personae
-----------------

- **XamarinAndroidNativeDebugTest** this repo, and a Visual Studio solution file of the same name
- **NativeHello** : A C++ project for Android (reachable from C#/CLR via PInvoke and SWIG bridge)
- **NativeHelloApp** : A C# Xamarin.Android project that simply calls into the native library to get back a string

Verify that a Debug build of the app builds and you can debug natively
----------------------------------------------------------------------

- In Visual Studio, right click on the **NativeHelloApp** project and select *Set as Startup Project*
- Attach an Android device to your PC and verify that Visual Studio sees it
- Select a *Debug* build configuration and select a architecture platform for the build (*ARM, ARM64, etc*) that matches the attached Android device
- Now open the **NativeHello.cpp** file that lives in the **NativeHello** project, and set a breakpoint in the `getPlatformABI()` function
- Open the Properties of the **NativeHelloApp**, switch to the *Android Options* tab, and change the *Debugger* to be C++
- Now build and debug the **NativeHelloApp** on your Android device and verify that you can hit the breakpoint you set in C++

Attempt to create a Release AAB bundle of the app
-------------------------------------------------

Once you are satistifed that the **NativeHelloApp** works and is natively debuggable, now we show tht you cannot create a Release AAB bundle of the app without hitting the error of the erroneously included `gdbserver`.

- Switch your build Configuration to *Release* with Platform *Any CPU*
- From the *Build* menu, choose *Batch Build*
- In the batch build popup, checkmark all the **NativeHello** project *Release* variants : (*ARM, ARM64, x64, x86*) and first click *Clean*
- After clean, go back to the batch build popup, make sure all the projects are still checked and click *Rebuild*
- After all the architectures are built, right-click on the **NativeHelloApp** project and choose *Rebuild*
- After the final build, right-click on the **NativeHelloApp** project and choose *Archive...*

Note that you will see the following failure: `Files under lib/ must have .so extension, found 'lib/x86/gdbserver'.`

Futile attempt #1 to work-around this failure to bundle
-------------------------------------------------------

In the **NativeHelloApp** Properties, under the *Android Options* tab (for a Release configuration), uncheck the *Enable developer instrumentation (debugging and profiling)*.  (NOTE that this  not particularly clear/discoverable what this checkbox is for exactly, but it does change the `<DebugSymbols>` in the project file which would lead us to suspect it has something more to do with PDB generation than whether `gdbsever` is bundled)

Repeat the Release AAB bundle process in the above section, and you should still see the same bundling failure.

Attempt #2 which gets the AAB bundle working, to the exclusion of native debugging
----------------------------------------------------------------------------------

In the **NativeHelloApp** Project *References*, remove the reference to the **NativeHello** project.

Repeat the Release AAB bundle process, and this time it should complete with SUCCESS!  But the price of this success is that we no longer get a bundled `gdbserver` for *Debug* build configurations.  To verify this new problem:

- Switch the build Configuration to *Debug* and choose a Platform arch that matches your attached Android device (e.g., *ARM, ARM64, etc*)
- Manually clean and rebuild the **NativeHello** project
- Manually clean and rebuild the **NativeHelloApp** project
- This time when you debug the **NativeHelloApp** project, you will see the C++ debugger hit the breakpoint you set in the **NativeHello.cpp** file
- At the CLI, you can verify that the debug APK (in WSL: `./NativeHelloApp/bin/Debug/com.companyname.nativehelloapp-Signed.apk`) that was built does NOT have any `gdbserver` included

So unfortunately we are stuck in a "Catch-22" situation here betweeen the unwanted inclusion of `gdbserver` in *Release* bundles, and the lack of `gdbserver` in *Debug* builds.

Questions
---------

So obviously this is a contrived simple example, but it illustrates a real issue we are having in our published Xamarin.Forms/Xamarin.Android app(s).  We have native-compiled libraries across all Android supported arch+ABIs, and we need the ability to debug into this layer for debug builds, as well as 

- Are we are overlooking some simple project fix that will unstick us from this Catch-22 situation?
- Could the inclusion of `gdbserver` in official *Release* APKs (non-AAB bundling procedure) be deemed a security risk?
- Is the above batch build process and the inclusion of the four arch+ABIs `nativeLibrary.so` files in the Xamarin.Android app the "recommended best practice" for building this type of "hybrid" managed/native Android app from Visual Studio?

An MSBuild fix that appears to Work For Us
------------------------------------------

So we have taken the liberty to sleuth into MSBuild and have hit upon a potential solution that appears to work just fine for us.

The problem appears to happen in the MSBuild file [Microsoft.Cpp.Android.targets](https://github.com/xamarin/xamarin-android/blob/master/src/Xamarin.Android.Build.Tasks/MSBuild/Xamarin/Android/Xamarin.Android.Common/ImportAfter/Microsoft.Cpp.Android.targets#L73).

The following patch gets us unstuck for both building a Release AAB bundle, while also not losing the ability to perform native debugging for Debug builds.

```
--- Microsoft.Cpp.Android.targets       2020-08-11 09:05:14.115831100 -0700
+++ "/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/MSBuild/Xamarin/Android/Xamarin.Android.Common/ImportAfter/Microsoft.Cpp.Android.targets"                                                  2020-08-11 09:09:31.283389000 -0700
@@ -66,7 +68,7 @@
         <Error Text="Native library references target platform $(NativeLibraryAbi) which is not supported by this project. Configured supported ABIs are: $(AndroidSupportedAbis)."
                Condition="$(AndroidSupportedAbis.Contains('$(NativeLibraryAbi)').ToString().ToLowerInvariant()) == 'false'" />

-        <ItemGroup Condition="'@(NativeLibraryPaths)' != ''">
+        <ItemGroup Condition="'$(Configuration)' == 'Debug' And '@(NativeLibraryPaths)' != ''">
             <AndroidNativeLibrary Include="@(NativeLibraryPaths)"
                                   Condition="'%(Extension)' == '.so' Or '%(FileName)' == 'gdbserver'">
                 <Abi>$(NativeLibraryAbi)</Abi>
@@ -76,6 +78,16 @@
                <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
             </Content>
         </ItemGroup>
+        <ItemGroup Condition="'$(Configuration)' != 'Debug' And '@(NativeLibraryPaths)' != ''">
+            <AndroidNativeLibrary Include="@(NativeLibraryPaths)"
+                                  Condition="'%(Extension)' == '.so'">
+                <Abi>$(NativeLibraryAbi)</Abi>
+            </AndroidNativeLibrary>
+            <Content Include="@(NativeLibraryPaths)"
+                     Condition="'%(Extension)' == '.so'">
+                <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
+            </Content>
+        </ItemGroup>

     </Target>
```
