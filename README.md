# libsequence

implement the sequence for sqlite

## How to use it

in the shell, you can load it using the command:

.load mypath/libsequence.dll

or using the select function in any tool like intellij <br />
select load_extension('mypath/libsequence.dll')

select seq_init('seq1',10)  -- create a sequence named seq1 with the initial value 10, increment will be 1 <br />
select seq_init('seq1',10,2) -- same as above but increment is specified as 2

seq_init creates the sequence if it does not exists, but it also initializes the sequence if it exists.

select seq_nextval('seq1') -- get the next value of the sequence seq1 <br />
select seq_currval('seq1') -- get the current value of the sequence seq1

Like with Oracle, seq_nextval must be called once in the session, otherwise seq_currval raises an error

select seq_drop('seq1') -- drop the sequence seq1

## How to compile it

the cmake file was done for mingw, I guess it is easy to support other compilers, please feel free to provide a request if you have another compiler.

put your cmake and mingw bin folders in your path, like below for windows <br />
**set** *PATH=C:\cmake\bin;C:\MINGW\BIN;%PATH%*

**c:\>** *cd path\to\libsequence*

create a build folder in the libsequence directory <br />
**c:\path\to\libsequence>** *mkdir build* <br />
**c:\path\to\libsequence>** *cd build* <br />

**c:\path\to\libsequence\build>** *cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..* <br />
**c:\path\to\libsequence\build>** *mingw32-make*

it will produce this output

Scanning dependencies of target sequence <br />
[ 50%] Building C object CMakeFiles/sequence.dir/sequence.c.obj <br />
[100%] Linking C shared library libsequence.dll <br />
[100%] Built target sequence

