// Copyright (c) 2013-2016 Trillek contributors. See AUTHORS.txt for details
// Licensed under the terms of the LGPLv3. See licenses/lgpl-3.0.txt

#pragma once

#include <list>
#include <vector>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "multiton.hpp"

namespace tec {
	class MeshFile;
	//typedef Multiton<std::string, std::shared_ptr<MeshFile>> MeshMap;

	// Vertex data used for rendering or other purposes.
	struct VertexData {
		VertexData() = default;
		VertexData(double x, double y, double z, double r, double g, double b, double u = 0.0, double v = 0.0) :
			position(x, y, z), color(r, g, b, 1.0f), uv(u, v) {}
		glm::vec3 position{ 0.f, 0.f, 0.f };
		glm::vec4 color{ 1.f, 1.f, 1.f, 1.f };
		glm::vec3 normal{ 0.f, 0.f, 0.f };
		glm::vec2 uv{ 0.f, 0.f };
		glm::vec4 bone_weights{ 0.f, 0.f, 0.f, 0.f }; // Used for animation.
		glm::uvec4 bone_indices{ 0, 0, 0, 0 }; // Used for animation.
	};

	struct MaterialGroup final {
		std::size_t start{ 0 };
		std::size_t count{ 0 };
		std::string material_name;
		std::list<std::string> textures;
	};

	struct ObjectGroup final {
		std::vector<unsigned int> indices;
		std::vector<MaterialGroup> material_groups;
	};

	struct Mesh final {
		std::vector<VertexData> verts;
		std::vector<std::unique_ptr<ObjectGroup>> object_groups;
	};

	class MeshFile {
	public:
		MeshFile() = default;
		virtual ~MeshFile() = default;
		MeshFile(MeshFile const&) = delete;
		MeshFile(MeshFile&&) = default;
		MeshFile& operator=(MeshFile const&) = delete;
		MeshFile& operator=(MeshFile&&) = default;

		/* It's safe to return a raw pointer, as long the person using the raw
		 * pointer remembers that it isn't allowed to live longer than the
		 * MeshFile itself, and that it is invalidated if any additional meshes
		 * are added to the MeshFile.
		 */
		Mesh* CreateMesh() {
			this->meshes.push_back(std::make_unique<Mesh>());
			return &**(this->meshes.end() - 1);
		}

		/**
		 * \brief Adds a mesh to this file. Ownership is transferred.
		 *
		 * \param[in] Mesh* mesh The mesh to add to this file.
		 * \return void
		 */
		void AddMesh(std::unique_ptr<Mesh> mesh) {
			this->meshes.push_back(std::move(mesh));
		}

		/* The returned pointer is invalidated if the MeshFile is destroyed or
		 * if any meshes are added to the MeshFile.
		 */
		Mesh* GetMesh(const std::size_t index) {
			if (index < this->meshes.size()) {
				return &*this->meshes.at(index);
			}

			return nullptr;
		}

		/**
		 * \brief Returns the number of meshes in this file.
		 *
		 * \return size_t The number of meshes in this file.
		 */
		std::size_t GetMeshCount() const {
			return this->meshes.size();
		}

		const std::string GetName() const {
			return this->name;
		}
		void SetName(const std::string& _name) {
			this->name = _name;
		}

		bool IsDirty() const {
			return this->dirty;
		}
		/** \brief Mark dirty */
		void Invalidate() {
			this->dirty = true;
		}
		/** \brief Mark not dirty */
		void Validate() {
			this->dirty = false;
		}
	protected:
		std::vector<std::unique_ptr<Mesh>> meshes;
		std::string name{ "test" };
		bool dirty{ false };
	};
}
