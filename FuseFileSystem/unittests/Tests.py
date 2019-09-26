# RUN THIS FILE USING PYTHON3
# > python3 Tests.py
# manual unmount after use required
# > fusermount -u ../mount


import subprocess
import os

# constants
test_files = 'testfiles/'
mount_path = '../mount/'

# some useful methods
def setup():
    print('----setup_file system-------')
    subprocess.call(['../mkfs.myfs', '../container.bin',test_files + 'shakespeare.txt'])

def mount():
    print('--------mount---------------')
    subprocess.call(['../mount.myfs', '../container.bin', '../log.txt', mount_path])

def copy_file(filename):
	fh_original = open(test_files + filename, 'r')
	fh_new = open(mount_path + filename, 'w')
	fh_new.write(fh_original.read())
	fh_original.close()
	fh_new.close()

def remove_file(filename):
	os.remove(mount_path + filename)
	
def clear():
	for file in os.listdir(mount_path):
		try:
			os.remove(mount_path + file)
		except:
			print('error removing file')

# generic test output that takes in a test name and a 'boolean'
def test(name, value):
    prt = name + ': '
    if(value):
        prt += 'success'
    else:
        prt += 'fail'
    print(prt)
    


setup()
mount()
print('--------tests--------------')

stat1 = os.stat(test_files + 'shakespeare.txt')
stat2 = os.stat(mount_path + 'shakespeare.txt')

test('test modified ctime', stat1.st_ctime < stat2.st_ctime)
test('test modified atime', stat1.st_atime < stat2.st_atime)

# initiating 64 read file handlers
fileHandler = []
for i in range(64):
    fileHandler.append(open(mount_path + 'shakespeare.txt', 'r'))

# trying to open a 65th file handler
tooManyFileHandlers = 0
try:
    open(mount_path + '/shakespeare.txt', 'r')
except:
    tooManyFileHandlers = 1
test('test failing to open a 65th file handler',tooManyFileHandlers)


# close all file handlers
for fh in fileHandler:
    fh.close()

#test whether appending to a file works correctly
fh = open(mount_path + 'allemeineentchen.txt','w+')
fh.write('alle meine')
fh.write(' entchen')
fh.close()
fh = os.open(mount_path + 'allemeineentchen.txt', os.O_WRONLY)
os.write(fh,bytes('alle keine','utf-8'))
os.close(fh)
with open(mount_path + 'allemeineentchen.txt', 'r') as read:
	text = read.read()
	print(text)
	test('test appening to an existing file', text == 'alle keine entchen')

nonexistentfile = 0
try:
	remove_file('nonexistentfile.txt')
except:
	nonexistentfile = 1
test('test error removing nonexistent file', nonexistentfile == 1)

clear()

# copying over 64 files
for file in os.listdir(test_files):
	copy_file(file)

test('test copying 64 files', len(os.listdir(mount_path)) == 64)

toomanyfiles = 0
try:
	open(mount_path + 'errorfile.txt','w+')
except:
	toomanyfiles = 1
test('test error on trying to create 65th file',toomanyfiles)

clear()

fh = open(mount_path + 'shakespeare.txt','w')
fh.write('teil1|')
fh.close()
fh = open(mount_path + 'shakespeare.txt','r')

copy_file('weird_text.txt')
fh1 = os.open(mount_path + 'shakespeare.txt', os.O_RDWR | os.O_DIRECT)
os.lseek(fh1, 0, os.SEEK_END)
os.write(fh1,bytes('|teil2','utf-8'))
os.close(fh1)

text = fh.read()
test('test separating data blocks of file',text == 'teil1||teil2')

