import wave
import numpy
import os
import struct
from matplotlib import pyplot

# reference
# Python: write a wav file into numpy float array
# https://stackoverflow.com/questions/16778878/python-write-a-wav-file-into-numpy-float-array

# python 3.11.4 wave - read and write wav files (https://docs.python.org/3/library/wave.html)
# Note that this does not include files using WAVE_FORMAT_EXTENSIBLE even if the subformat is PCM. !

# unpack
# https://stackoverflow.com/questions/3783677/how-to-read-integers-from-a-file-that-are-24bit-and-little-endian-using-python

# some setting
gPlotStart = 0
gPlotEnd = 128
gPlotPadding = 192000
gDataName = "rimsky_192k_2ch_16b_short_FRONT_LEFT"
gGoldenDir = "0808/192k/LOST_none_COMP_none"
gLostDir = "0809/192k/LOST_inter_sample4x_freq1x_COMP_none"
gCompDir = "0809/192k/LOST_inter_sample4x_freq1x_COMP_intp"

def unpack_s24bit_new(inputBytes):
	val = struct.unpack('<i', inputBytes + (b"\0" if inputBytes[2] < 128 else b"\xff"))
	return val

def unpack_s24bit(inputBytes):
	val = inputBytes[0] | (inputBytes[1] << 8) | (inputBytes[2] << 16)
	if val > 0x00800000 :
		val = -(0x01000000 - val)
	return val

def loadWav(loadtype=0) :

	if loadtype == 0:
		fileName = "output/{}/MY_{}.wav".format(gGoldenDir, gDataName)
	elif loadtype == 1:
		fileName = "output/{}/MY_{}.wav".format(gLostDir, gDataName)
	elif loadtype == 2:
		fileName = "output/{}/MY_{}.wav".format(gCompDir, gDataName)
	else :
		fileName = "output/LOST_none_COMP_none/wav/MY_{}.wav".format(gDataName)
	ifile = wave.open(fileName)
	samples = ifile.getnframes()
	fileinfo = ifile.getparams()
	audio = ifile.readframes(samples)

	if fileinfo.sampwidth == 1 :
		ret_numpy_array = numpy.frombuffer(audio, dtype=numpy.uint8)
	if fileinfo.sampwidth == 2 :
		ret_numpy_array = numpy.frombuffer(audio, dtype=numpy.int16)
	elif fileinfo.sampwidth == 3 :
		ret_numpy_array = numpy.array([], dtype=numpy.int32)
		for i in range(0, int(gPlotEnd+gPlotPadding*4)*3) : 
			ret_numpy_array = numpy.append(ret_numpy_array, [unpack_s24bit_new(audio[i*3:i*3+3])])
	else :
		ret_numpy_array = numpy.frombuffer(audio, dtype=numpy.int32)

	return fileinfo, ret_numpy_array

def showPCM(start = gPlotStart, end = gPlotEnd, padding = gPlotPadding) :

	fig, ax = pyplot.subplots(3,1)
	fig.suptitle('{} pcm data'.format(gDataName))
	fig.set_figwidth(15)
	fig.set_figheight(9)
	fig.subplots_adjust(left=0.05, right=0.95)

	dataSta = start
	dataEnd = end
	ax[0].plot(range(start, end), audio_golden_int32[dataSta:dataEnd], 'g', linewidth=0.5, label='golden')
	ax[0].plot(range(start, end), audio_lost_int32[dataSta:dataEnd], 'k-o', linewidth=0.5, label='lost', markersize=2)
	ax[0].plot(range(start, end), audio_proc_int32[dataSta:dataEnd], 'r', linewidth=0.5, label='comp')
	ax[0].set_title('sample from {} to {}'.format(dataSta, dataEnd))
	ax[0].legend(loc='upper right')

	dataSta = start+padding
	dataEnd = end+padding
	ax[1].plot(range(dataSta, dataEnd), audio_golden_int32[dataSta:dataEnd], 'g', linewidth=0.5, label='golden')
	ax[1].plot(range(dataSta, dataEnd), audio_lost_int32[dataSta:dataEnd], 'k-o', linewidth=0.5, label='lost', markersize=2)
	ax[1].plot(range(dataSta, dataEnd), audio_proc_int32[dataSta:dataEnd], 'r', linewidth=0.5, label='comp')
	ax[1].set_title('sample from {} to {}'.format(dataSta, dataEnd))
	ax[1].legend(loc='upper right')

	dataSta = start+padding*2
	dataEnd = end+padding*2
	ax[2].plot(range(dataSta, dataEnd), audio_golden_int32[dataSta:dataEnd], 'g', linewidth=0.5, label='golden')
	ax[2].plot(range(dataSta, dataEnd), audio_lost_int32[dataSta:dataEnd], 'k-o', linewidth=0.5, label='lost', markersize=2)
	ax[2].plot(range(dataSta, dataEnd), audio_proc_int32[dataSta:dataEnd], 'r', linewidth=0.5, label='comp')
	ax[2].set_title('sample from {} to {}'.format(dataSta, dataEnd))
	ax[2].legend(loc='upper right')
	pyplot.show()

if __name__ == "__main__":
	
	global audio_golden_fileinfo
	global audio_golden_int32
	global audio_lost_fileinfo
	global audio_lost_int32
	global audio_proc_fileinfo
	global audio_proc_int32

	# load data
	audio_golden_fileinfo, audio_golden_int32 = loadWav(0)
	audio_lost_fileinfo, audio_lost_int32 = loadWav(1)
	audio_proc_fileinfo, audio_proc_int32 = loadWav(2)

	# plot
	# gPlotPadding = audio_golden_fileinfo.framerate - 500
	# gPlotPadding = 0
	showPCM(gPlotStart, gPlotEnd, gPlotPadding)

	'''
	tmpdata = b"\xfe\xff\xff"
	# print(type(tmpdata[2]))

	if tmpdata[2] < 128 :
		print("hi")
	else : 
		print("what")

	tmpval = struct.unpack('<i', tmpdata + (b"\0" if tmpdata[2] < 128 else b"\xff"))
	# tmpval = struct.unpack('<i', tmpdata)
	print(tmpval)

	quit()
	'''
