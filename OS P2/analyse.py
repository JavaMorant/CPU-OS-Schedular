import sys
import re
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import matplotlib.patches as mpatches
import numpy as np

# Accept the log file as a command line argument
log_file = sys.argv[1]

# Create lists to store the data
data_list = []
pids = []
procs = []
wake_times = []
run_times = []
end_times = []

# Parse the data
with open(log_file, "rb") as file:
    for line in file:
        if line.startswith(b"$$"):
            m = re.search(
                b"timeslice summary for pid ([0-9]+) \\((.*)\\) : queued at ([0-9]+), ran at ([0-9]+), ended at ([0-9]+)",
                line,
            )
            pid, proc, wake, run, end = int(m[1]), m[2], int(m[3]), int(m[4]), int(m[5])
            data_list.append(
                {
                    "pid": pid,
                    "proc": proc.decode(),
                    "wake": wake,
                    "run": run,
                    "end": end,
                }
            )

# Create a DataFrame from the list of dictionaries
data = pd.DataFrame(data_list)


# Calculate wait times
data["wait_time"] = (data["run"] - data["wake"]) / 1000
# Save to CSV file
data.to_csv("process_wait_times.csv", index=False)

# Analysis calculations
grouped_data = data.groupby("pid")
for pid, group in grouped_data:
    total_run_time = group["end"].sub(group["run"]).sum()
    avg_run_time = total_run_time / len(group)
    total_wait_time = group["run"].sub(group["wake"]).sum()
    avg_wait_time = total_wait_time / len(group)
    min_run_time = group["end"].sub(group["run"]).min()
    max_run_time = group["end"].sub(group["run"]).max()

    print(
        f"PID {pid} ({group['proc'].iloc[0]}): Total Run Time = {total_run_time / 1000} ms, Average Run Time = {avg_run_time / 1000} ms"
    )
    print(
        f"PID {pid} ({group['proc'].iloc[0]}): Total Wait Time = {total_wait_time / 1000} ms, Average Wait Time = {avg_wait_time / 1000} ms"
    )
    print(
        f"PID {pid} ({group['proc'].iloc[0]}): Min/Max Run Time = {min_run_time / 1000}/{max_run_time / 1000} ms"
    )

# Set a seaborn style
sns.set(style="whitegrid")

# Create subplots
fig, axes = plt.subplots(3, 2, figsize=(15, 20))
fig.subplots_adjust(hspace=1)

# Assuming COLOURS is a list of color names
COLOURS = [
    "red",
    "blue",
    "green",
    "yellow",
    "purple",
    "orange",
    "brown",
    "pink",
    "grey",
    "cyan",
]
pid_colours = {
    pid: COLOURS[i % len(COLOURS)] for i, pid in enumerate(data["pid"].unique())
}
# Gantt Chart with Custom Colors and Legend
axes[0, 0].set_title("Gantt Chart of Processes with Custom Colors")
patches = []
for pid, color in pid_colours.items():
    axes[0, 0].barh(
        y=pid,
        width=(data[data["pid"] == pid]["end"] - data[data["pid"] == pid]["run"])
        / 1000,
        left=data[data["pid"] == pid]["run"] / 1000,
        label=f"PID {pid}",
        color=color,
    )
    patches.append(mpatches.Patch(color=color, label=f"PID {pid}"))
axes[0, 0].set_xlabel("Time (s)")
axes[0, 0].set_ylabel("PID")
axes[0, 0].legend(handles=patches)

# Pie Chart for Wait Time by Processes
axes[0, 1].set_title("Pie Chart of Wait Time by Processes")
# Calculate the total wait time for each PID
total_wait_times = grouped_data.apply(lambda x: (x["run"] - x["wake"]).sum() / 1000)
# Colors for the pie chart
colors = [
    "red",
    "blue",
    "green",
    "yellow",
    "purple",
    "orange",
    "brown",
    "pink",
    "grey",
    "cyan",
]
# Create a pie chart
axes[0, 1].pie(
    total_wait_times,
    labels=[f"PID {pid}" for pid in total_wait_times.index],
    colors=colors,
    autopct="%1.1f%%",
    startangle=140,
)
# Ensure the pie chart is drawn as a circle
axes[0, 1].axis("equal")


# Total Run Time and Wait Time per Process
bar_width = 0.35
bar_positions = list(grouped_data.groups.keys())
axes[1, 0].bar(
    bar_positions,
    grouped_data.apply(lambda x: (x["end"] - x["run"]).sum() / 1000),
    width=bar_width,
    label="Total Run Time",
    color="skyblue",
)

axes[1, 0].bar(
    [pos + bar_width for pos in bar_positions],
    grouped_data.apply(lambda x: (x["run"] - x["wake"]).sum() / 1000),
    width=bar_width,
    label="Total Wait Time",
    color="lightcoral",
    hatch="/",
)

axes[1, 0].set_xlabel("PID")
axes[1, 0].set_ylabel("Time (ms)")
axes[1, 0].set_title("Total Run Time and Wait Time per Process")
axes[1, 0].set_xticks([pos + bar_width / 2 for pos in bar_positions])
#axes[1, 0].set_xticklabels(
 #   [f"PID {pid} ({group['proc'].iloc[0]})" for pid, group in grouped_data]
#)
axes[1, 0].legend()

# Additional Clustered Bar Chart
axes[1, 1].set_title("Clustered Bar Chart")
legend_patches = [
    mpatches.Patch(color="skyblue", label="Total Run Time"),
    mpatches.Patch(color="lightcoral", hatch="/", label="Total Wait Time"),
]
axes[1, 1].legend(handles=legend_patches)
axes[1, 1].barh(
    y=bar_positions,
    width=grouped_data.apply(lambda x: (x["end"] - x["run"]).sum() / 1000),
    left=0,
    color="skyblue",
)
axes[1, 1].barh(
    y=bar_positions,
    width=grouped_data.apply(lambda x: (x["run"] - x["wake"]).sum() / 1000),
    left=grouped_data.apply(lambda x: (x["end"] - x["run"]).sum() / 1000),
    color="lightcoral",
    hatch="/",
)
axes[1, 1].set_yticks([])

# Histogram of Wait Times for Each Process
axes[2, 1].set_title("Histogram of Wait Times for Each Process")
# Group data by process identifier (assuming 'pid' is the process identifier)
grouped_by_pid = data.groupby("pid")
# Define colors for each process histogram (assuming there are not more processes than colors)
colors = [
    "red",
    "blue",
    "green",
    "yellow",
    "purple",
    "orange",
    "brown",
    "pink",
    "grey",
    "cyan",
]
for (pid, group), color in zip(grouped_by_pid, colors):
    wait_times = (group["run"] - group["wake"]) / 1000
    axes[2, 1].hist(
        wait_times,
        bins=20,
        color=color,
        edgecolor="black",
        alpha=0.5,
        label=f"PID {pid}",
    )
axes[2, 1].set_xlabel("Wait Time (s)")
axes[2, 1].set_ylabel("Frequency")
axes[2, 1].legend()

# Histogram of Overall Run Times
axes[2, 0].set_title("Histogram of Overall Run Times")
axes[2, 0].hist(
    (data["end"] - data["run"]) / 1000,
    bins=20,
    color="skyblue",
    edgecolor="black",
    label="Run Time",
)
axes[2, 0].set_xlabel("Run Time (s)")
axes[2, 0].set_ylabel("Frequency")
axes[2, 0].legend()

# Save the combined figure
plt.savefig("scheduler_combined_plots.png")
plt.show()

# Calculate overall summary statistics
overall_mean_wait_time = (data["run"] - data["wake"]).mean() / 1000
overall_mean_run_time = (data["end"] - data["run"]).mean() / 1000

print("Overall Mean response time:", overall_mean_wait_time, "ms")
print("Overall Mean run time:", overall_mean_run_time, "ms")
print("Overall Mean turnaround time:", overall_mean_wait_time + overall_mean_run_time, "ms")
print("Overall Mean throughput:", 1000 / (overall_mean_wait_time + overall_mean_run_time), "processes per second")
print("CPU Utilisation:", overall_mean_run_time / (overall_mean_wait_time + overall_mean_run_time) * 100, "%")
print("Idle Time:", overall_mean_wait_time / (overall_mean_wait_time + overall_mean_run_time) * 100, "%")
print("Context Switches:", len(data))