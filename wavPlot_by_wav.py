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

# some setting
gPlotStart = 0
gPlostEnd = 1000
gDataName = "beemoved_96k_2ch_24b_short_FRONT_LEFT"

def unpack_s24bit(bytes):
	val = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16)
	if val > 0x00800000 :
		val = -(0x01000000 - val)
	return val

def loadWav(loadtype=0) :

	if loadtype == 0:
		fileName = "output/LOST_none_COMP_none/wav/MY_{}.wav".format(gDataName)
	elif loadtype == 1:
		fileName = "output/LOST_cont_sample4x_freq128x_COMP_none/wav/MY_{}.wav".format(gDataName)
	elif loadtype == 2:
		fileName = "output/LOST_cont_sample4x_freq128x_COMP_intp/wav/MY_{}.wav".format(gDataName)
	else :
		fileName = "output/LOST_none_COMP_none/wav/MY_{}.wav".format(gDataName)
	ifile = wave.open(fileName)
	samples = ifile.getnframes()
	fileinfo = ifile.getparams()
	audio = ifile.readframes(samples)

	if fileinfo.sampwidth == 1 or fileinfo.sampwidth == 2 :
		ret_numpy_array = numpy.frombuffer(audio, dtype=numpy.int32)
	elif fileinfo.sampwidth == 3 :
		ret_numpy_array = numpy.array([], dtype=numpy.int32)
		for i in range(0, int(gPlostEnd*3)) : 
			ret_numpy_array = numpy.append(ret_numpy_array, [unpack_s24bit(audio[i*3:i*3+3])])
	else :
		ret_numpy_array = numpy.frombuffer(audio, dtype=numpy.int32)

	return ret_numpy_array

def showPCM(start = gPlotStart, end = gPlostEnd) :
	fig = pyplot.figure(1)
	fig.set_figwidth(15)
	fig.subplots_adjust(left=0.05, right=0.95)
	pyplot.title('{} pcm data'.format(gDataName))
	pyplot.plot(range(start, end), audio_golden_int32[start:end], 'g', linewidth=0.5, label='golden')
	pyplot.plot(range(start, end), audio_lost_int32[start:end], 'k', linewidth=0.5, label='lost')
	pyplot.plot(range(start, end), audio_proc_int32[start:end], 'r', linewidth=0.5, label='comp')
	pyplot.legend()
	pyplot.show()

if __name__ == "__main__":
	global audio_golden_int32
	global audio_lost_int32
	global audio_proc_int32
	audio_golden_int32 = loadWav(0)
	audio_lost_int32 = loadWav(1)
	audio_proc_int32 = loadWav(2)
	showPCM()
