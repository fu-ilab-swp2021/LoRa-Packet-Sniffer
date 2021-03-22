import sys
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.ticker as ticker
import time
import argparse
from pathlib import Path

def plotData(dataframe, pdf, timeslot):

    # Creating Plot with 6 subplots
    fig, ax = plt.subplots(nrows=2, ncols=3, figsize=(18, 12))

    # sort data by time
    dataframe = dataframe.sort_values("Time")

    #keys = ["Time", "ChannelFreq", "RSSI", "SNR", "MType", "DevAddr", "ADR", "ADRACKReq", "ACK", "FCnt", "FOptsLen" , "FOpts", "FPort"]

    # Cutting the dataframe according to the given timeslot and plot number of messages per hour
    dataframe = plot_time(fig, ax, dataframe, timeslot)

    # Plotting other plots:
    rssi_values = dataframe["RSSI"].tolist()
    plot_rssi(ax, rssi_values)

    snr_values = dataframe["SNR"].tolist()
    plot_snr(ax, snr_values)

    mtype_values = dataframe["MType"].tolist()
    plot_message_types(ax, mtype_values)

    devaddr_values = dataframe["DevAddr"].tolist()
    channelfreq_values = dataframe["ChannelFreq"].tolist()
    plot_message_number(fig, ax, devaddr_values, channelfreq_values)

    ack_values = dataframe["ACK"].tolist()
    plot_acknowledgements(ax, ack_values)

    # Saving the figure to the pdf file and close the plots
    pdf.savefig()
    plt.close("all")

    # Return dataframe to use for additional plots
    return dataframe


def plot_rssi(axis, values):
    # Plot in the top left corner:
    # RSSI distribution
    rssi_counted = dict((x, values.count(x)) for x in set(values))
    rssi_x = []
    rssi_y = []

    #creating x and y axis array for the plot
    for i in range(-120, 1):
        if i in rssi_counted:
            rssi_x.append(i)
            rssi_y.append(rssi_counted[i])
        else:
            rssi_x.append(i)
            rssi_y.append(0)

    axis[0,0].set_yticks(range(0, 21, 5))

    axis[0,0].set_title("RSSI distribution", fontweight="bold")
    axis[0,0].set_xlabel("RSSI in db", fontweight="bold")
    axis[0,0].set_ylabel("Number of messages", fontweight="bold")

    axis[0,0].bar(rssi_x, rssi_y)
    return
    


def plot_snr(axis, values):
    # Plot in the middle at the top:
    # SNR distribution:
    snr_counted = dict((x,values.count(x)) for x in set(values))
    snr_x = []
    snr_y = []

    #creating x and y axis array for the plot
    for i in range(-20, 11):
        if i in snr_counted:
            snr_x.append(i)
            snr_y.append(snr_counted[i])
        else:
            snr_x.append(i)
            snr_y.append(0)

    axis[0,1].set_title("SNR distribution", fontweight="bold")
    axis[0,1].set_xlabel("Signal to Noise Ratio", fontweight="bold")
    axis[0,1].set_ylabel("Number of messages", fontweight="bold")
    axis[0,1].bar(snr_x, snr_y, color="goldenrod")
    return


def plot_message_types(axis, values):
    # Number of messages per message type

    # counting amount of occurences of the message types
    mtype_counted = dict((x,values.count(x)) for x in set(values))
    mtype_x = []
    mtype_y = []

    #creating y axis array for the plot
    for key in mtype_counted:
        mtype_x.append(key)
        mtype_y.append(mtype_counted[key])

    mtype_xticks = [1,2,3]
    for i in range(0, 5, 2):
        if i not in mtype_x:
            if i == 0:
                mtype_x.insert(0, 0)
                mtype_y.insert(0, 0)
            if i == 2:
                mtype_x.insert(1, 2)
                mtype_y.insert(1, 0)
            if i == 4:
                mtype_x.insert(2, 4)
                mtype_y.insert(2, 0)

    for i in range(len(mtype_x)):
        if mtype_x[i] == 0:
            mtype_x[i] = "Join Request"
        if mtype_x[i] == 2:
            mtype_x[i] = "Unconfirmed\n Data Up Packet"
        if mtype_x[i] == 4:
            mtype_x[i] = "Confirmed\n Data Up Packet"

    axis[0,2].set_xticks(mtype_xticks)
    axis[0,2].set_xticklabels(mtype_x)
    axis[0,2].yaxis.set_major_locator(ticker.MaxNLocator(integer=True))

    axis[0,2].set_title("Number of messages per message type", fontweight="bold")
    axis[0,2].set_xlabel("Type of message", fontweight="bold")
    axis[0,2].set_ylabel("Number of messages", fontweight="bold")

    axis[0,2].bar(mtype_xticks, mtype_y, color="indigo")
    
    return



def plot_message_number(figure, axis, devaddr_values, channelfreq_values):
    # Plot in the bottom left corner:
    # Number of messages for the 5 (or less) most active devices (only in the general overview):
    # (only for more information on a specific device)
    devaddr_counted = dict((x,devaddr_values.count(x)) for x in set(devaddr_values))
    number_of_devices = len(devaddr_counted)
    if(number_of_devices > 5):
        figure.suptitle("Overview of all devices (all time)", fontweight="bold", fontsize=20)
        devaddr_sorted_dict = sorted(list(devaddr_counted.items()), key=lambda x: x[1], reverse=True)
        devaddr_x, devaddr_y = zip(*devaddr_sorted_dict)

        

        axis[1,0].set_xticks(range(5))
        axis[1,0].set_xticklabels(devaddr_x[:5])

        axis[1,0].set_title("Number of messages of the 5 most active devices", fontweight="bold")
        axis[1,0].set_xlabel("Device Addresses (Hexadecimal)", fontweight="bold")
        axis[1,0].set_ylabel("Number of massages", fontweight="bold")

        axis[1,0].bar(range(5), devaddr_y[:5], color="teal")
    elif number_of_devices > 1:
        figure.suptitle(f"Overview of all {number_of_devices} devices", fontweight="bold", fontsize=20)
        devaddr_sorted_dict = sorted(list(devaddr_counted.items()), key=lambda x: x[1], reverse=True)
        devaddr_x, devaddr_y = zip(*devaddr_sorted_dict)

        

        axis[1,0].set_xticks(range(number_of_devices))
        axis[1,0].set_xticklabels(devaddr_x[:number_of_devices])

        axis[1,0].set_title(f"Number of messages of the {number_of_devices} most active devices", fontweight="bold")
        axis[1,0].set_xlabel("Device Addresses (Hexadecimal)", fontweight="bold")
        axis[1,0].set_ylabel("Number of massages", fontweight="bold")

        axis[1,0].bar(range(number_of_devices), devaddr_y[:number_of_devices], color="teal")
    else:
        figure.suptitle(f"Overview of the device with the device address {list(devaddr_counted)[0]}", fontweight="bold", fontsize=20)

        frequencies = [867100000, 867300000, 867500000, 867700000, 867900000, 868100000, 868300000, 868500000]
        channelfreq_counted = dict((x,channelfreq_values.count(x)) for x in set(channelfreq_values))
        channelfreq_x = []
        channelfreq_y = []

        #creating x and y axis array for the plot
        for i in frequencies:
            if i in channelfreq_counted:
                channelfreq_x.append(i)
                channelfreq_y.append(channelfreq_counted[i])
            else:
                channelfreq_x.append(i)
                channelfreq_y.append(0)

        axis[1,0].set_title("Number of messages received per frequency ", fontweight="bold")
        axis[1,0].set_xlabel("Channel frequency in mHz", fontweight="bold")
        axis[1,0].set_ylabel("Number of messages", fontweight="bold")

        axis[1,0].bar(range(len(frequencies)), channelfreq_y, color="teal")
        axis[1,0].set_xticks(range(len(frequencies)))
        axis[1,0].set_xticklabels([str((x/1000000)) for x in frequencies])

        return



def plot_acknowledgements(axis, values):
    # Plot in the middle at the bottom:
    # Acknowledgements vs. No acknowledgements
    ack_counted = dict((x,values.count(x)) for x in set(values))
    ack_x = []
    ack_y = []

    #creating y axis array for the plot
    for key in ack_counted:
        ack_x.append(key)
        ack_y.append(ack_counted[key])

    for i in range(len(ack_x)):
        if ack_x[i] == 0:
            ack_x[i] = "No Acknowledgement"
        if ack_x[i] == 1:
            ack_x[i] = "Acknowledgement"

    axis[1,1].set_title("Acknowledgement vs. No Acknowledgement", fontweight="bold")

    axis[1,1].pie(ack_y, (0,0), ack_x, wedgeprops=dict(width=0.5), labeldistance=None, pctdistance=0.75, autopct='%1.1f%%', startangle=90, colors=["xkcd:crimson", "green"], textprops={"fontweight": "bold"})
    axis[1,1].axis("equal")
    axis[1,1].legend(loc="lower left")

    return



def plot_time(figure, axis, dataframe, timeslot):
    # Plot in the bottom right corner:
    # Packets received per hour since start of recording:

    time_values = dataframe["Time"].tolist()

    #sorting timestamp and converting to seconds
    difference = 0
    time_values = sorted(time_values)
    for i in range(len(time_values)):
        time_values[i] = int(time_values[i] / 1000)
        if i == 0:
            difference = time_values[0]
        time_values[i] = time_values[i] - difference

    
    time_dict = {}

    for i in range(len(time_values)):
        hour = int(time_values[i] / 3600)
        if hour in time_dict:
            time_dict[hour] += 1
        else:
            time_dict[hour] = 0

    time_x = []
    time_y = []

    for i in range(len(time_dict)+1):
        if i in time_dict:
            time_x.append(i)
            time_y.append(time_dict[i])

    if(timeslot != None):

        print(timeslot[1], len(time_x))

        if(timeslot[1] > len(time_x)):
            sys.exit("Attention: second timeslot value is higher than the actual number of hours recorded in the data")

        cut_before = 0
        cut_after = 0
        for i in time_y[:timeslot[0]]:
            cut_before += i
        for j in time_y[timeslot[1]:]:
            cut_after += i

        dataframe.reset_index(drop=True, inplace=True)
        dataframe = dataframe.truncate(before=cut_before, after=(len(dataframe)-cut_after))

        time_x = time_x[timeslot[0]:timeslot[1]]
        time_y = time_y[timeslot[0]:timeslot[1]]

    axis[1,2].set_title("Packets received per hour since start of recording", fontweight="bold")
    axis[1,2].set_xlabel("Hour since start of recording", fontweight="bold")
    axis[1,2].set_ylabel("Number of received packets", fontweight="bold")
    axis[1,2].yaxis.set_major_locator(ticker.MaxNLocator(integer=True))
    axis[1,2].xaxis.set_major_locator(ticker.MaxNLocator(integer=True))

    axis[1,2].bar(time_x, time_y, color="xkcd:plum")

    return dataframe



if __name__ == "__main__":

    # Init the argument parser, add and parse arguments
    parser = argparse.ArgumentParser()

    parser.add_argument("input_file", type=str, help="csv file or path to a csv file to get the data from")
    parser.add_argument("output_file", type=str, help="name for a new file, existing pdf file or path to an existing pdf file to write the output into")
    parser.add_argument("--devaddr", "-d", nargs="*", default="", help="Device address(es) to get more detailed information on the given device(s)")
    parser.add_argument("--time", "-t", type=int, nargs=2, help="")

    args = vars(parser.parse_args())

    # Reading and formatting input filename
    input_filename = args["input_file"]
    if not ".csv" in input_filename:
        input_filename = input_filename + ".csv"
    input_filename = Path(input_filename)

    # Reading data from csv file
    data = pd.read_csv(input_filename)

    # Reading and formatting output filename
    output_filename = args["output_file"]
    if not ".pdf" in output_filename:
        output_filename = output_filename + ".pdf"
    output_filename = Path(output_filename)

    # Creating PdfPages Object to use for output
    pdf = PdfPages(output_filename)

    # Time restirction:
    timeslot = args["time"]
    if timeslot != None:
        if timeslot[0] < 0:
            sys.exit("Attention: first timeslot value can't be lower than zero")
        if timeslot[0] > timeslot[1]:
            sys.exit("Attention: first timeslot value can not be higher than the second timeslot value")
    
    # Plotting general overview and output to pdf file 
    data = plotData(data, pdf, timeslot)


    # Getting device addresses and compare with input argument "devaddr"
    devaddresses = data["DevAddr"].tolist()
    device_names = args["devaddr"]

    if device_names != "":
        for name in device_names:
            if name in devaddresses:
                # Getting data coming only from the given device
                specific_device = data.loc[data["DevAddr"] == name]
                # Plot data for the given device
                plotData(specific_device, pdf, None)
    else:
        print("No specific device(s) given")

    pdf.close()
