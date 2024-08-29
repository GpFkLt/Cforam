import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

schemeL = ['LO13', 'KM19', 'AFN17', 'Ours', 'OursOpt']
legend_labels = ['LO13', 'KM19', 'AFN17', 'Cforam', 'Cforam+']
para = ["LAN", "WAN"]
markL = ['X', 'o', 's', 'd', 'p']
colorL = ['red', 'lime', 'blue', 'orange', 'purple']
XList = [1 << i for i in range(6, 21, 2)]

# 设置图像的显示参数
plt.rcParams.update({
    "legend.fancybox": False,
    "legend.frameon": True,
    "font.serif": ["Times"],
    "font.size": 23,
    "axes.titlesize": 23, # Title font size
    "axes.labelsize": 23  # Label font size
})

# 创建三张并列的子图
fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(24, 6))

for i in range(len(schemeL)):
    csv_file = f'{schemeL[i]}/Result/DiffDB_{para[0]}.csv'
    df = pd.read_csv(csv_file)
    
    # 第一个图：amortized_access_bandwidth (B) in LAN
    ax1.plot(XList[:len(df['amortized_access_bandwidth (B)'])], df['amortized_access_bandwidth (B)'] / 1024, 
             marker=markL[i], color=colorL[i], linestyle='dashed', 
             label=legend_labels[i], markersize=15)

    # 第二个图：amortized_access_time (s) in LAN
    ax2.plot(XList[:len(df['amortized_access_time (s)'])], df['amortized_access_time (s)'] * 1000, 
             marker=markL[i], color=colorL[i], linestyle='dashed', 
             label=legend_labels[i], markersize=15)

    # 第三个图：amortized_access_time (s) in WAN
    csv_file_wan = f'{schemeL[i]}/Result/DiffDB_{para[1]}.csv'
    df_wan = pd.read_csv(csv_file_wan)
    ax3.plot(XList[:len(df_wan['amortized_access_time (s)'])], df_wan['amortized_access_time (s)'] * 1000, 
             marker=markL[i], color=colorL[i], linestyle='dashed', 
             label=legend_labels[i], markersize=15)

for ax in [ax1, ax2]:
    ax.set_xscale('log', base=2)
    ax.set_yscale('log', base=2)
    ax.set_xticks(XList)
    ax.grid(True)

ax3.set_xscale('log', base=2)
ax3.set_yscale('log', base=2)
ax3.set_xticks(XList[:5])
ax3.grid(True)

ax1.set_title('Bandwidth Consumption', pad=10)
ax1.set_xlabel('Database Size (# of 32-byte blocks)')
ax1.set_ylabel('Bandwidth (KB)')

ax2.set_title('LAN Setting', pad=10)
ax2.set_xlabel('Database Size (# of 32-byte blocks)')
ax2.set_ylabel('Time (ms)')

ax3.set_title('WAN Setting', pad=10)
ax3.set_xlabel('Database Size (# of 32-byte blocks)')
ax3.set_ylabel('Time (ms)')


# 将图例放在三个图的上方
handles, labels = ax1.get_legend_handles_labels()
fig.legend(handles, labels, loc='upper center', ncol=5, bbox_to_anchor=(0.5, 0.96))

# 调整布局和保存图像
plt.tight_layout(rect=[0, 0, 1, 0.88])
plt.savefig('oram_diff_db.pdf', bbox_inches='tight', dpi=600)
plt.close()
plt.show()
