import sys
import re

# setup the plot
import matplotlib.pyplot as plt

COLOURS = ['tab:blue',
          'tab:orange',
          'tab:green',
          'tab:red',
          'tab:purple',
          'tab:brown',
          'tab:pink',
          'tab:gray',
          'tab:olive',
          'tab:cyan']

plt_y = []
plt_w = []
plt_x = []
plt_c = []


# parse the data

pids = dict()
start_time = None

wait_time_samples = []

for line in open(sys.argv[1], 'rb'):
    if line.startswith(b'$$'):
        # parse the line
        m = re.search(b'timeslice summary for pid ([0-9]+) \\((.*)\\) : queued at ([0-9]+), ran at ([0-9]+), ended at ([0-9]+)', line)
        pid, proc, wake, run, end = int(m[1]), m[2], int(m[3]), int(m[4]), int(m[5])
        pids[pid] = proc.decode()

        # get a start time
        if start_time is None:
            start_time = wake

        wake -= start_time
        run  -= start_time
        end  -= start_time

        waiting_time = run - wake
        running_time = end - run

        # do something interesting with these figures
        wait_time_samples.append(waiting_time)

        # add to plot
        plt_x.append(run / 1000)
        plt_w.append(running_time / 1000)
        plt_y.append(1)
        plt_c.append(COLOURS[pid - 1])


# Suggestion: maybe do this per-process instead?
print('Mean wait time:',
      sum(wait_time_samples) / len(wait_time_samples) / 1000,
      'ms')
        

# complete the plot        
# Legend
import matplotlib.patches as mpatches
patches = []
for i, c in enumerate(COLOURS):
    if i + 1 in pids:
        l = pids[i + 1]
        patches.append(mpatches.Patch(color=c, label=l))
plt.legend(handles=patches)

# plot bars
plt.barh(y=plt_y, width=plt_w, left=plt_x, color=plt_c)
plt.yticks([])
plt.tight_layout()
plt.show()
