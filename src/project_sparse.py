# Python script to project the ThnSparse
import ROOT
import yaml
import numpy as np
import argparse
import os

def project_thnsparse(config_path):
    # Load configuration
    with open(config_path, "r") as config_file:
        config = yaml.safe_load(config_file)
    config_projection = config["projection"]  # Get the projection configuration
    config_axes = config_projection["sparse_axes"]  # Get the axes configuration

    # Prepare output directory
    output_dir = config.get("outdir", "results/")
    output_dir_projection = os.path.join(output_dir, "projections")
    os.makedirs(output_dir_projection, exist_ok=True)

    # Load the ThnSparse from the ROOT file
    # if download is True, use the downloaded file, otherwise use the default path
    if config["do_download"]:
        infile = f"{config['outdir']}/downloaded_data/AnalysisResults_{config['out_label']}.root"
    else:
        infile = config_projection["input_sparse"]

    root_file = ROOT.TFile(infile)
    thnsparse = root_file.Get(config_projection['sparse_path'])
    print(f"Loaded ThnSparse from '{infile}' with name '{config_projection['sparse_path']}' and total entries: {thnsparse.GetEntries()}")

    # Define the projection axes and ranges
    hsign = thnsparse.Projection(config_axes['sign'])
    thnsparse.GetAxis(config_axes['sign']).SetRangeUser(-1, 0)  # Set the range for the 'sign' axis to select ULS (unlike-sign) pairs
    thnsparse_uls = thnsparse.Projection(config_axes['mass'], config_axes['pt'])
    thnsparse_uls.SetDirectory(0)
    thnsparse.GetAxis(config_axes['sign']).SetRangeUser(0, 2)  # Set the range for the 'sign' axis to select LS (like-sign) pairs
    thnsparse_ls = thnsparse.Projection(config_axes['mass'], config_axes['pt'])
    thnsparse_ls.SetDirectory(0)  # Detach the ThnSparse from the file to prevent it from being deleted when the file is closed

    # Project the ThnSparse onto the specified axes
    pt_bins = config_projection["pt_bins"]
    mass_bins = config_projection["mass_bins"]
    hmass_ls, hmass_uls, hdiff = [], [], []
    thnsparse_ls.GetXaxis().SetRangeUser(pt_bins[0], pt_bins[-1]) 
    hmass_ls.append(thnsparse_ls.ProjectionY())
    hmass_ls[-1].SetDirectory(0)
    thnsparse_uls.GetXaxis().SetRangeUser(pt_bins[0], pt_bins[-1])
    hmass_uls.append(thnsparse_uls.ProjectionY())
    hmass_uls[-1].SetDirectory(0)
    hmass_ls[-1].SetName("hmass_ls_pt_int")
    hmass_uls[-1].SetName("hmass_uls_pt_int")
    hdiff.append(hmass_uls[-1].Clone("hdiff_pt_int"))
    hdiff[-1].Add(hmass_ls[-1], -1)
    hdiff[-1].SetName("hdiff_pt_int")
    hdiff[-1].SetDirectory(0)
    hdiff[-1].Rebin(10)
    for ipt, (pt_min, pt_max) in enumerate(zip(pt_bins[:-1], pt_bins[1:])):
        thnsparse_ls.GetXaxis().SetRangeUser(pt_min, pt_max)
        thnsparse_uls.GetXaxis().SetRangeUser(pt_min, pt_max)
        hmass_ls.append(thnsparse_ls.ProjectionY())
        hmass_ls[-1].SetDirectory(0)
        hmass_uls.append(thnsparse_uls.ProjectionY())
        hmass_uls[-1].SetDirectory(0)
        hdiff.append(hmass_uls[-1].Clone(f"hdiff_pt_{pt_min}_{pt_max}"))
        hdiff[-1].Add(hmass_ls[-1], -1)
        hdiff[-1].SetName(f"hdiff_pt_{pt_min}_{pt_max}")
        hdiff[-1].Rebin(10)
        hdiff[-1].SetDirectory(0)

    # Save the projected histograms to a ROOT file
    outfile = ROOT.TFile(f"{output_dir_projection}/projected_histograms.root", "RECREATE")
    hmass_ls[0].Write()
    hmass_uls[0].Write()
    hdiff[0].Write()
    for ipt, (pt_min, pt_max) in enumerate(zip(pt_bins[:-1], pt_bins[1:])):
        ipt += 1  # Start from 1 to avoid overwriting the inclusive projection
        hmass_ls[ipt].SetName(f"hmass_ls_pt_{pt_min}_{pt_max}")
        hmass_uls[ipt].SetName(f"hmass_uls_pt_{pt_min}_{pt_max}")
        hmass_ls[ipt].Write()
        hmass_uls[ipt].Write()
        hdiff[ipt].Write()
    thnsparse_ls.Write("thnsparse_ls")
    thnsparse_uls.Write("thnsparse_uls")
    hsign.Write("hsign")
    outfile.Close()
    print(f"Projected histogram saved to '{output_dir_projection}/projected_histograms.root'")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Arguments')
    parser.add_argument('config', type=str, help='Path to the configuration file')
    args = parser.parse_args()
    project_thnsparse(args.config)
