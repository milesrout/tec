// Copyright (c) 2013-2016 Trillek contributors. See AUTHORS.txt for details
// Licensed under the terms of the LGPLv3. See licenses/lgpl-3.0.txt

#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "filesystem.hpp"
#include "resources/mesh.hpp"

namespace tec {
	class MD5Mesh final : public MeshFile {
	public:
		/*****************************/
		/* MD5Mesh helper structures */
		/*****************************/
		struct Joint {
			/**
			 * \brief Compute the joint's quaternion W component.
			 *
			 * \return void
			 */
			void ComputeW();

			std::string name{ "" }; // The name of the joint
			int parent{ -1 }; // index
			glm::vec3 position{ 0.f, 0.f, 0.f }; // Transformed position.
			glm::quat orientation{ 0.f, 0.f, 0.f, 1.f }; // Quaternion
			glm::mat4 bind_pose{ 0.f };
			glm::mat4 bind_pose_inverse{ 0.f };
		};

		struct Vertex {
			int startWeight{ 0 }; // index
			unsigned int weight_count{ 0 };
			glm::vec2 uv{ 0.f, 0.f }; // Texture coordinates
			glm::vec3 position{ 0.f, 0.f, 0.f }; // Calculated position (cached for later use)
			glm::vec3 normal{ 0.f, 0.f, 0.f }; // Calculated normal (cached for later use)
		};

		struct Triangle {
			int verts[3]{ 0, 0, 0 }; // index
		};

		struct Weight {
			int joint{ 0 }; // index
			float bias{ 0.f }; // 0-1
			glm::vec3 position{ 0.f, 0.f, 0.f };
		};

		// Holds information about each mesh inside the file.
		struct InternalMesh {
			std::string shader; // MTR or texture filename.
			std::vector<Vertex> verts;
			std::vector<Triangle> tris;
			std::vector<Weight> weights;
		};

		MD5Mesh(std::string name, std::istream& is);

		/**
		 * \brief Calculates the final vertex positions based on the bind-pose skeleton.
		 *
		 * There isn't a return as the processing will just do nothing if the
		 * parse data was default objects.
		 * \return void
		 */
		void CalculateVertexPositions();

		/**
		 * \brief Calculates the vertex normals based on the bind-pose skeleton and mesh tris.
		 *
		 * There isn't a return as the processing will just do nothing if the
		 * parse data was default objects.
		 * \return void
		 */
		void CalculateVertexNormals();

		/**
		 * \brief Updates the meshgroups index list based from the loaded mesh groups.
		 *
		 * There isn't a return as the processing will just do nothing if the
		 * parse data was default objects.
		 * \return void
		 */
		void UpdateIndexList();

		// Used for MD5Anim::CheckMesh().
		friend class MD5Anim;
	private:
		std::vector<InternalMesh> meshes_internal;
		std::vector<Joint> joints;
	};
}
